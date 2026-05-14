#include "helpers.hpp"
#include "../s4/utils.hpp"
#include "../device.hpp"
#include "../utils/utils.hpp"
#include "../s2/np2lt.hpp"
#include <fstream>
#include <filesystem>
#include <future>

Helpers::Parameters::Parameters () {

    process_fn = [this](CaptureData ts) -> std::pair<torch::Tensor, bool> {

        // Save to results
        Helpers::_Result result;
        result.label      = ts.label;
        result.prediction = preds.item<int>();
        result.index      = _PDF.process_count;
        result.batch_id   = ts.batch_id;
        result.reward     = loss.item<double>();

        results.push_back(result);

        ++_PDF.process_count;
        return {loss, true};
    };

    save_masks_thread_running.store (true, std::memory_order_release);

    // Create the save mask thread
    save_mask_thread = std::thread([this]() {
        torch::Tensor mask;
        int mask_count = 0;
        std::vector<torch::Tensor> batch_masks;
        while (save_masks_thread_running.load(std::memory_order_acquire)) {
            // Check if there are masks to save
            if (!masks_queue.try_dequeue(mask)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            batch_masks.push_back(mask.detach().cpu()); // each mask is [n, H, W]
            if (batch_masks.size() >= this->n_samples) {
                // stack into a single [n*20, H, W] tensor
                auto bms = torch::cat(batch_masks, 0);

                // launch async task to save the batch of masks, so that the saving process doesn't block the main thread
                std::string filepath = this->collect_data_directory + "/mask_" + std::to_string(mask_count) + ".pt";
                std::async(std::launch::async, [bms, filepath]() {
                    torch::save(bms, filepath);
                });

                batch_masks.clear();
                ++mask_count;
            }
        };
    });
}

Helpers::Parameters::~Parameters () {
    // Wait until all masks have been saved
    while (masks_queue.size_approx() > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    save_masks_thread_running.store(false, std::memory_order_release);
    if (save_mask_thread.joinable()) {
        save_mask_thread.join();
    }
}

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

void Helpers::Run::Setup_Scheduler (
    Parameters &params,
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
        2716,       /* Horizontal Resolution */
        FULLSCREEN, 
        NO_TARGET_FPS,
        30,         /* Target FPS (if needed) */

        480,        /* Camera Height */
        640,        /* Camera Width */
        params.Camera.exposure_time_us,     /* Exposure Time (us) */
        1,          /* Binning Horizontal */
        1,          /* Binning Vertical */
        3,          /* Line Trigger */
        params.Camera.partitioning, /* Use Zones */
        4,          /* Number of Zones */
        10,         /* Zone Offset Height */
        50,         /* Zone Size */
        true,       /* Use Centering */
        0,          /* Offset X */
        0,          /* Offset Y */
        8,          /* Pixel Format: 8 for Mono8, 10 for Mono */

        2 * Height * params.upscale_amount,   /* PEncoder Height */
        3 * Width  * params.upscale_amount,   /* PEncoder Width */
        params.num_levels,
        params.plm_device_enum, /* PLM Device Enum */

        &opt,       /* Optimizer */

        params.process_fn  /* Processing function */

    );

    //scheduler.EnableSampleImageCapture();
    scheduler.DisableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);
    scheduler.EnableLabelQueueing();
    scheduler.EnableBlendMode();    /* For use with DLP/PLM system, disable if only PLM */
    scheduler.EnableFullScreenSubTextures();

    if (dist.get_name() == "categorical") {
        std::cout << "INFO: [e23] Using Categorical distribution for the model.\n";
        scheduler.EnableCategoricalMode(); // For Categorical distribution
    } else if (dist.get_name() == "normal") {
        std::cout << "INFO: [e23] Using Normal distribution for the model.\n";
    }
    else {
        std::cout << "INFO: [e23] Using unknown distribution (" << dist.get_name() << ") for the model.\n";
    }

    scheduler.SetSubShaderThreshold(params.sub_shader_threshold);
    scheduler.SetBatchSize(params.n_batch_size);
    scheduler.SetPosterization(params.use_posterization);
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
    ofs << "Compute Time (s): " << compute_time_s << '\n';
    ofs << "Samples Total: " << samples_total << '\n';
    ofs << "Samples Correct: " << samples_correct << '\n';
    ofs << "Accuracy: " << accuracy << '\n';
    ofs << "Entropy: " << entropy << '\n';
    ofs << "Loss: " << loss << '\n';
    ofs << "----------------------------------------\n";

    ofs.close();
}

Helpers::Run::Performance Helpers::Run::Evaluate (
    Parameters &params,
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    Data::Batch &batch
) {
    // Clear the PDF data
    params._PDF.clear_data();

    // Get the batch size from batch
    int64_t start_time = Utils::GetCurrentTime_s();
    int batch_size = batch.textures.size();

    for (int i = 0; i < params.n_samples; ++i) {
        torch::Tensor action = eval_fn.sample(scheduler.maximum_number_of_frames_in_image); // [20, H, W]

        if (params.collect_data) {
            // Save action into collect_data_masks
            params.masks_queue.enqueue(action.cpu());
        }

        action = Utils::UpscaleTensor(
            action,
            params.upscale_amount
        );

        scheduler.SetTextureFromTensorTiled (
            action
        );

        for (int j = 0; j < batch_size; ++j) {
            int label = batch.labels[j];
            scheduler.SetLabel(label, 20);
            scheduler.SetSubTextures(batch.textures[j], 0);
            Iterate(params, scheduler);
            ++params.steps;
        }

        //TakeScreenshot("screenshot.png");

    }

    // Wait
    scheduler.SetVSYNC_Marker();
    scheduler.WaitVSYNC_Diff(2);

    eval_fn.squash();
    double loss = eval_fn.update();
    int64_t end_time = Utils::GetCurrentTime_s ();

    int64_t delta = end_time - start_time;
    
    Helpers::Run::Performance perf;

    perf.compute_time_s = static_cast<double>(delta);
    perf.samples_total   = params._PDF.total.load(std::memory_order_acquire);
    perf.samples_correct = params._PDF.correct.load(std::memory_order_acquire);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy        = eval_fn.entropy();
    perf.loss           = loss;

    return perf;
}

Helpers::Run::Performance Helpers::Run::Evaluate (
    Parameters &params,
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    std::vector<Data::Batch> &batches
) {
    int64_t start_time = Utils::GetCurrentTime_s();

    Helpers::Run::Performance perf;

    //
    if (params.n_samples_update_rate != -1) {
        if (params.epoch_counter != 0 && params.epoch_counter % params.n_samples_update_rate == 0) {
            params.n_samples += params.n_samples_update_amount;
        }
    }

    for (auto &batch : batches) {
        auto pp = Evaluate(params, scheduler, eval_fn, batch);
        perf.samples_total   += pp.samples_total;
        perf.samples_correct += pp.samples_correct;
        perf.entropy        += pp.entropy;;
        perf.loss           += pp.loss;
    }

    // Update parameters
    params.epoch_counter++;

    int64_t end_time   = Utils::GetCurrentTime_s();
    int64_t delta = end_time - start_time;

    auto _results = params.GetResults(0);

    perf.samples_total = _results.size();
    perf.samples_correct = 0;
    for (const auto &res : _results) {
        if (res.label == res.prediction) {
            ++perf.samples_correct;
        }
    }

    perf.compute_time_s = static_cast<double>(delta);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy       /= static_cast<double>(batches.size());
    perf.loss          /= static_cast<double>(batches.size());
    return perf;
}

Helpers::Run::Performance Helpers::Run::Inference (
    Parameters &params,
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    Data::Batch &batch
) {
    // Clear the PDF data
    params._PDF.clear_data();

    // Get the batch size from batch
    int64_t start_time = Utils::GetCurrentTime_s();
    int batch_size = batch.textures.size();

    torch::Tensor action = eval_fn.base(scheduler.maximum_number_of_frames_in_image);

    action = Utils::UpscaleTensor(
        action,
        params.upscale_amount
    );

    scheduler.SetTextureFromTensorTiled (
        action
    );

    for (int j = 0; j < batch_size; ++j) {
        int label = batch.labels[j];
        scheduler.SetLabel(label, 20);
        scheduler.SetSubTextures(batch.textures[j], 0);
        Iterate(params, scheduler);
        ++params.steps;
    }

    // Wait
    scheduler.SetVSYNC_Marker();
    scheduler.WaitVSYNC_Diff(2);

    eval_fn.squash();
    double loss = eval_fn.loss();
    int64_t end_time = Utils::GetCurrentTime_s();

    int64_t delta = end_time - start_time;
    Helpers::Run::Performance perf;
    perf.compute_time_s = static_cast<double>(delta);
    perf.samples_total   = params._PDF.total.load(std::memory_order_acquire);
    perf.samples_correct = params._PDF.correct.load(std::memory_order_acquire);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy        = eval_fn.entropy();
    perf.loss           = loss;

    return perf;
}

Helpers::Run::Performance Helpers::Run::Inference (
    Parameters &params,
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn,
    std::vector<Data::Batch> &batches
) {
    int64_t start_time = Utils::GetCurrentTime_s();

    Helpers::Run::Performance perf;
    perf.samples_total   = 0;
    perf.samples_correct = 0;
    perf.entropy        = 0.0;
    perf.loss           = 0.0;


    for (auto &batch : batches) {
        auto pp = Inference(params, scheduler, eval_fn, batch);
        perf.samples_total   += pp.samples_total;
        perf.samples_correct += pp.samples_correct;
        perf.entropy        += pp.entropy;;
        perf.loss           += pp.loss;
    }

    int64_t end_time   = Utils::GetCurrentTime_s();
    int64_t delta = end_time - start_time;

    perf.compute_time_s = static_cast<double>(delta);
    perf.accuracy        = (static_cast<double>(perf.samples_correct) / static_cast<double>(perf.samples_total)) * 100.0;
    perf.entropy       /= static_cast<double>(batches.size());
    perf.loss          /= static_cast<double>(batches.size());
    return perf;
}

Helpers::_pdf::_pdf () {

    save_dir = "";
    save_image_thread_running.store(true, std::memory_order_release);
    num_images_per_batch = 20; // Save 20 images per .pt file
    // Create the save image thread

    save_image_thread = std::thread([this]() {
        torch::Tensor img;
        int image_idx = 0;
        std::vector<torch::Tensor> batch_images;
        while (save_image_thread_running.load(std::memory_order_acquire)) {
            // Load img from queue
            if (!this->image_queue.try_dequeue(img)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }


            batch_images.push_back(img.detach().cpu());
            if (batch_images.size() >= this->num_images_per_batch) {
                auto bi = torch::stack(batch_images, 0); // [num_images_per_batch, H, W]
                // Save the batch of images as a single .pt file
                std::string filepath = this->save_dir + "/batch_" + std::to_string(image_idx) + ".pt";

                // launch async task to save the batch of images, so that the saving process doesn't block the main thread
                std::async(std::launch::async, [bi, filepath]() {
                    torch::save(bi, filepath);
                });
                batch_images.clear();
                ++image_idx;
            }
        }

        // If any images are left in the batch_images vector, save them as well
        if (!batch_images.empty()) {
            auto bi = torch::stack(batch_images, 0); // [num_images_per_batch, H, W]
            std::string filepath = this->save_dir + "/batch_" + std::to_string(image_idx) + ".pt";
            torch::save(bi, filepath);
        }

        std::cout << "INFO: [Helpers::_pdf] Save image thread exiting. Total images saved: " << image_idx * this->num_images_per_batch + batch_images.size() << "\n";

    });
}

Helpers::_pdf::~_pdf () {
    // Wait until queue is clear
    while (image_queue.size_approx() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    save_image_thread_running.store(false, std::memory_order_release);
    if (save_image_thread.joinable()) {
        save_image_thread.join();
    }
}

void Helpers::_pdf::clear_data () {
    correct.store(0, std::memory_order_release);
    total.store(0, std::memory_order_release);

    // Clear label counts and frequencies
    for (int i = 0; i < 10; ++i) {
        label_counts[i] = 0;
        label_freq[i]   = 0;
    }
}

void Helpers::Run::Iterate (Parameters &params, Scheduler2 &scheduler) {
    for (int i = 0; i < params.n_iterate_amount; ++i) {
        scheduler.DrawTextureToScreenCentered ();

        scheduler.SetVSYNC_Marker();
        scheduler.WaitVSYNC_Diff(1);
    }
    scheduler.ReadFromCamera ();
}

Helpers::Run::Performance::Performance () {
    compute_time_s = 0.0;
    samples_total   = 0;
    samples_correct = 0;
    entropy        = 0.0;
    accuracy       = 0.0;
    loss           = 0.0;
}

Helpers::Checkpoint::Checkpoint () {
    Epoch = 0;
    config_file = "";
    mask = torch::Tensor();
}

void Helpers::Checkpoint::Save (const std::string &directory) {
    // Create directory if it doesn't exist

    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::string sub_directory = directory + "/epoch_" + std::to_string(Epoch);
    if (!std::filesystem::exists(sub_directory)) {
        std::filesystem::create_directories(sub_directory);
    }

    // config file
    std::string config_path = sub_directory + "/config.txt";
    std::ofstream ofs(config_path);

    // Load the original config file and save it to the checkpoint directory's config file
    std::ifstream ifs(config_file);
    if (ifs && ofs) {

        // First line denotes that it is a checkpoint config file
        ofs << CHECKPOINT_DENOTE << '\n';

        std::string line;
        while (std::getline(ifs, line)) {
            ofs << line << '\n';
        }
        
        // Extend the config file with checkpoint info
        ofs << "CheckpointEpoch " << Epoch << '\n';

        // Save path to mask file
        ofs << "MaskLocation " << sub_directory + "/mask.pt" << '\n';

        ofs.close ();
    }

    // Save the mask tensor
    std::string mask_path = sub_directory + "/mask.pt";
    torch::save({mask}, mask_path);
}

void Helpers::Parameters::SaveCollectedMasks () {
    if (collect_data == false)
        return;

    if (collect_data_masks.empty()) {
        std::cout << "INFO: [SaveCollectedMasks] No masks collected, skipping save.\n";
        return;
    }

    // Stack the collected masks into a single tensor
    torch::Tensor all_masks = torch::stack(collect_data_masks); // [N, 20, H, W]

    // Save the tensor to a file dictated in directory collected_masks_directory
    std::string save_path = collect_data_directory + "/masks.pt";
    torch::save(all_masks, save_path);
    std::cout << "INFO: [SaveCollectedMasks] Saved " << collect_data_masks.size() << " masks to " << save_path << '\n';

}