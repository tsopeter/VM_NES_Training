#include "helpers.hpp"
#include "../s4/utils.hpp"
#include "../device.hpp"
#include "../utils/utils.hpp"
#include <fstream>

void Helpers::Data::Delete (std::vector<Batch> &batches) {
    for (auto &batch : batches) {
        for (auto &tex : batch.textures) {
            UnloadTexture(tex);
        }
        batch.textures.clear();
        batch.labels.clear();
    }
    batches.clear();
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get_Training () {
    return Get (
        Parameters::n_training_samples,
        Parameters::n_batch_size,
        s2_DataTypes::TRAIN,
        Parameters::n_padding
    );
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get_Validation () {
    return Get (
        Parameters::n_validation_samples,
        Parameters::n_validation_batch_size,
        s2_DataTypes::VALID,
        Parameters::n_padding
    );
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get_Test () {
    return Get (
        Parameters::n_test_samples,
        Parameters::n_test_batch_size,
        s2_DataTypes::TEST,
        Parameters::n_padding
    );
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get (
    int n_data_points,
    int batch_size,
    s2_DataTypes dtype,
    int padding
) {
    s2_Dataloader data_loader {dataset_path};
    auto data = data_loader.load(dtype, n_data_points);
    std::cout << "INFO: [Helpers::Data::Get] Loaded data with " << data.len() << " samples.\n";

    std::vector<Helpers::Data::Batch> batches;
    batches.resize(n_data_points / batch_size);

    for (int i = 0; i < n_data_points; i += batch_size) {
        for (int j = 0; j < batch_size; ++j) {
            auto [d, l] = data[i + j];
            // Process the data

            auto li = l.item<int>();

            d = 255.0 - d;

            // Pad the image if padding > 0
            if (padding > 0) {
                d = torch::nn::functional::pad(
                    d.unsqueeze(0).unsqueeze(0), // [1, 1, H, W]
                    torch::nn::functional::PadFuncOptions({padding, padding, padding, padding}).mode(torch::kConstant).value(255)
                ).squeeze(); // [H + 2*padding, W + 2*padding]
            }

            Image di = s4_Utils::TensorToImage(d);
            Texture ti = LoadTextureFromImage(di);
            SetTextureFilter(ti, TEXTURE_FILTER_BILINEAR);
            UnloadImage(di);

            batches[i / batch_size].textures.push_back(ti);
            batches[i / batch_size].labels.push_back(li);
        }

    }

    return batches;


}

void Helpers::Run::Setup_Scheduler (
    Scheduler2 &scheduler,
    s4_Optimizer &opt,
    Distributions::Definition &dist,
    int Height,
    int Width
) {
    // Setup the scheduler based on 
    scheduler.Start (
        0,          /* Monitor */
        1600,       /* Vertical Resolution */
        2560,       /* Horizontal Resolution */
        FULLSCREEN, 
        NO_TARGET_FPS,
        30,         /* Target FPS (if needed) */

        480,        /* Camera Height */
        640,        /* Camera Width */
        Parameters::Camera::exposure_time_us,     /* Exposure Time (us) */
        1,          /* Binning Horizontal */
        1,          /* Binning Vertical */
        3,          /* Line Trigger */
        Parameters::Camera::partitioning, /* Use Zones */
        4,          /* Number of Zones */
        60,         /* Zone Size */
        true,       /* Use Centering */
        0,          /* Offset X */
        0,          /* Offset Y */
        8,          /* Pixel Format: 8 for Mono8, 10 for Mono */

        2 * Height * Parameters::upscale_amount,   /* PEncoder Height */
        2 * Width  * Parameters::upscale_amount,   /* PEncoder Width */
        
        &opt,       /* Optimizer */

        Parameters::process_fn  /* Processing function */

    );

    scheduler.EnableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);
    scheduler.EnableLabelQueueing();
    scheduler.EnableBlendMode();    /* For use with DLP/PLM system, disable if only PLM */
    scheduler.EnableFullScreenSubTextures();

    if (dist.get_name() == "categorical") {
        std::cout << "INFO: [e23] Using Categorical distribution for the model.\n";
        scheduler.EnableCategoricalMode(); // For Categorical distribution
    } else if (dist.get_name() == "normal") {
        std::cout << "INFO: [e23] Using Normal distribution for the model.\n";
    } else if (dist.get_name() == "bernoulli" || dist.get_name() == "binary") {
        std::cout << "INFO: [e23] Using Binary distribution for the model.\n";
        scheduler.EnableBinaryMode(); // For Binary distribution
    }
    else {
        std::cout << "INFO: [e23] Using unknown distribution (" << dist.get_name() << ") for the model.\n";
    }

    scheduler.SetSubShaderThreshold(Parameters::sub_shader_threshold);
    scheduler.SetBatchSize(Parameters::n_batch_size);
}

void Helpers::Run::Performance::Save (
    const std::string &filename,
    int   Epoch,
    std::string message
) {
    std::ofstream ofs(filename, std::ios::app);

    ofs << "----------------------------------------\n";
    if (!message.empty()) {
        ofs << message << '\n';
    }
    ofs << "Epoch: " << Epoch << '\n';
    ofs << "Compute Time (ms): " << compute_time_ms << '\n';
    ofs << "Samples Total: " << samples_total << '\n';
    ofs << "Samples Correct: " << samples_correct << '\n';
    ofs << "Accuracy: " << accuracy << '\n';
    ofs << "Entropy: " << entropy << '\n';
    ofs << "Loss: " << loss << '\n';
    ofs << "----------------------------------------\n";

    ofs.close();
}

Helpers::Run::Performance Helpers::Run::Evaluate (
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    Data::Batch &batch
) {
    // Clear the PDF data
    Parameters::_PDF::clear_data();

    // Get the batch size from batch
    int64_t start_time = Utils::GetCurrentTime_us();
    int batch_size = batch.textures.size();

    for (int i = 0; i < Parameters::n_samples; ++i) {
        torch::Tensor action = eval_fn.sample(i);

        action = Utils::UpscaleTensor(
            action,
            Parameters::upscale_amount
        );

        scheduler.SetTextureFromTensorTiled (
            action
        );

        for (int j = 0; j < batch_size; ++j) {
            int label = batch.labels[j];
            scheduler.SetLabel(label, 20);
            scheduler.SetSubTextures(batch.textures[j], 0);
            Iterate(scheduler);
            ++Parameters::steps;
        }

    }

    // Wait
    scheduler.SetVSYNC_Marker();
    scheduler.WaitVSYNC_Diff(2);

    eval_fn.squash();
    double loss = scheduler.Update ();
    int64_t end_time = Utils::GetCurrentTime_us();

    int64_t delta = end_time - start_time;
    delta /= 1'000; // Convert to ms
    Helpers::Run::Performance perf;

    perf.compute_time_ms = static_cast<double>(delta);
    perf.samples_total   = Parameters::_PDF::total.load(std::memory_order_acquire);
    perf.samples_correct = Parameters::_PDF::correct.load(std::memory_order_acquire);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy        = eval_fn.entropy();
    perf.loss           = loss;

    return perf;
}

Helpers::Run::Performance Helpers::Run::Evaluate (
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    std::vector<Data::Batch> &batches
) {
    int64_t start_time = Utils::GetCurrentTime_us();

    Helpers::Run::Performance perf;

    for (auto &batch : batches) {
        auto pp = Evaluate(scheduler, eval_fn, batch);
        perf.samples_total   += pp.samples_total;
        perf.samples_correct += pp.samples_correct;
        perf.entropy        += pp.entropy;;
        perf.loss           += pp.loss;
    }

    int64_t end_time   = Utils::GetCurrentTime_us();
    int64_t delta = end_time - start_time;
    delta /= 1'000; // Convert to ms

    perf.compute_time_ms = static_cast<double>(delta);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy       /= static_cast<double>(batches.size());
    perf.loss          /= static_cast<double>(batches.size());
    return perf;
}

Helpers::Run::Performance Helpers::Run::Inference (
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    Data::Batch &batch
) {
    // Clear the PDF data
    Parameters::_PDF::clear_data();

    // Get the batch size from batch
    int64_t start_time = Utils::GetCurrentTime_us();
    int batch_size = batch.textures.size();

    for (int i = 0; i < Parameters::n_samples; ++i) {
        torch::Tensor action = eval_fn.base(i);

        action = Utils::UpscaleTensor(
            action,
            Parameters::upscale_amount
        );

        scheduler.SetTextureFromTensorTiled (
            action
        );

        for (int j = 0; j < batch_size; ++j) {
            int label = batch.labels[j];
            scheduler.SetLabel(label, 20);
            scheduler.SetSubTextures(batch.textures[j], 0);
            Iterate(scheduler);
            ++Parameters::steps;
        }

    }

    // Wait
    scheduler.SetVSYNC_Marker();
    scheduler.WaitVSYNC_Diff(2);

    eval_fn.squash();
    double loss = scheduler.Loss ();
    int64_t end_time = Utils::GetCurrentTime_us();

    int64_t delta = end_time - start_time;
    delta /= 1'000; // Convert to ms
    Helpers::Run::Performance perf;

    perf.compute_time_ms = static_cast<double>(delta);
    perf.samples_total   = Parameters::_PDF::total.load(std::memory_order_acquire);
    perf.samples_correct = Parameters::_PDF::correct.load(std::memory_order_acquire);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy        = eval_fn.entropy();
    perf.loss           = loss;

    return perf;
}

Helpers::Run::Performance Helpers::Run::Inference (
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    std::vector<Data::Batch> &batches
) {
    int64_t start_time = Utils::GetCurrentTime_us();

    Helpers::Run::Performance perf;

    for (auto &batch : batches) {
        auto pp = Inference(scheduler, eval_fn, batch);
        perf.samples_total   += pp.samples_total;
        perf.samples_correct += pp.samples_correct;
        perf.entropy        += pp.entropy;;
        perf.loss           += pp.loss;
    }

    int64_t end_time   = Utils::GetCurrentTime_us();
    int64_t delta = end_time - start_time;
    delta /= 1'000; // Convert to ms

    perf.compute_time_ms = static_cast<double>(delta);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy       /= static_cast<double>(batches.size());
    perf.loss          /= static_cast<double>(batches.size());
    return perf;
}

void Helpers::Parameters::_PDF::clear_data () {
    correct.store(0, std::memory_order_release);
    total.store(0, std::memory_order_release);
    process_count = 0;

    // Clear label counts and frequencies
    for (int i = 0; i < 10; ++i) {
        label_counts[i] = 0;
        label_freq[i]   = 0;
    }
}

void Helpers::Run::Iterate (Scheduler2 &scheduler) {
    for (int i = 0; i < Parameters::n_iterate_amount; ++i) {
        scheduler.DrawTextureToScreenCentered ();

        scheduler.SetVSYNC_Marker();
        scheduler.WaitVSYNC_Diff(1);
    }
    scheduler.ReadFromCamera ();
}