#include "helpers.hpp"
#include "../s4/utils.hpp"
#include "../device.hpp"
#include "../utils/utils.hpp"
#include "../s2/np2lt.hpp"
#include <fstream>
#include <filesystem>

Helpers::Parameters::Parameters () {

    process_fn = [this](CaptureData ts) -> std::pair<torch::Tensor, bool> {

        auto img = ts.full.to(DEVICE);
        auto l   = torch::tensor({ts.label}).to(DEVICE);

        std::cout << "DEBUG: [process_fn] Received image tensor of shape: " << img.sizes() << "\n";
        std::cout << "DEBUG: [process_fn] Mask is of shape: " << _PDF.masks.sizes() << "\n";

        img      = torch::clamp(img, 0, 255).to(torch::kFloat32);
        img      = torch::where(img < _PDF.min_limit, torch::zeros_like(img), img).to(torch::kFloat64);

        img      = (img  - _PDF.min_limit) * (255.0 / (255.0 - _PDF.min_limit));
        img      = torch::clamp(img, 0, 255) / 255.0f; // [0 - 1]

        auto q    = img.unsqueeze(0).unsqueeze(0); // [1, 1, H, W]
        auto sums = (q.unsqueeze(1) * _PDF.masks.to(img.device())).sum({2,3,4}); // [1, 10]
        sums = sums * _PDF.ratios.to(sums.device());

        if (_PDF.process_count % _PDF.save_iter == 0) {
            std::cout << "INFO: [process_fn] Processed " << _PDF.process_count << " samples so far.\n";
            auto img_img = img * 255.0f;
            img_img = img_img.contiguous().to(torch::kUInt8);

            auto Img = s4_Utils::TensorToImage(img_img);
            ExportImage(Img, "sample.bmp");
            UnloadImage(Img);
        } 

        auto preds = sums.argmax(1); // [1]

        if (_PDF.process_count % _PDF.accuracy_interval == 0) {
            if (preds.item<int>() == ts.label) {
                _PDF.correct.fetch_add(1, std::memory_order_release);
            }
            _PDF.total.fetch_add(1, std::memory_order_release);

            _PDF.label_counts[ts.label]++;
            _PDF.label_freq[preds.item<int>()]++;
        }

        auto targets = l.to(torch::kLong).to(sums.device()); // [1]


        auto loss = _PDF.loss_fn(sums, targets);  // [1]

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

std::vector<Helpers::Data::Batch> Helpers::Data::Get_Training (Parameters &params) {
    return Get (
        params,
        params.n_training_samples,
        params.n_batch_size,
        s2_DataTypes::TRAIN,
        params.n_padding,
        params.n_start_index
    );
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get_Validation (Parameters &params) {
    return Get (
        params,
        params.n_validation_samples,
        params.n_validation_batch_size,
        s2_DataTypes::VALID,
        params.n_padding,
        params.n_validation_start_index
    );
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get_Test (Parameters &params) {
    return Get (
        params,
        params.n_test_samples,
        params.n_test_batch_size,
        s2_DataTypes::TEST,
        params.n_padding,
        params.n_test_start_index
    );
}

std::vector<Helpers::Data::Batch> Helpers::Data::Get (
    Parameters &params,
    int n_data_points,
    int batch_size,
    s2_DataTypes dtype,
    int padding,
    int start_index
) {
    s2_Dataloader data_loader {params.Training.dataset_path};
    auto data = data_loader.load(dtype, start_index+n_data_points);
    std::cout << "INFO: [Helpers::Data::Get] Loaded data with " << data.len() << " samples.\n";

    switch (dtype) {
        case s2_DataTypes::TRAIN:
            std::cout << "INFO: [Helpers::Data::Get] Data Type: TRAIN\n";
            break;
        case s2_DataTypes::VALID:
            std::cout << "INFO: [Helpers::Data::Get] Data Type: VALID\n";
            break;
        case s2_DataTypes::TEST:
            std::cout << "INFO: [Helpers::Data::Get] Data Type: TEST\n";
            break;
    }

    std::cout << "INFO: [Helpers::Data::Get] Number of Data Points: " << n_data_points << "\n";
    std::cout << "INFO: [Helpers::Data::Get] Batch Size: " << batch_size << "\n";
    std::cout << "INFO: [Helpers::Data::Get] Start Index: " << start_index << "\n";

    std::vector<Helpers::Data::Batch> batches;
    batches.resize(n_data_points / batch_size);

    // If prewarped is set, load prewarped images from directory
    bool use_prewarped = false;
    torch::Tensor map_x = torch::Tensor();
    torch::Tensor map_y = torch::Tensor();
    if (!params.prewarped_directory.empty()) {
        std::cout << "INFO: [Helpers::Data::Get] Loading prewarped images from directory: " << params.prewarped_directory << "\n";
        use_prewarped = true;


        map_x = np2lt::f32(params.prewarped_directory + "/map_x.npy");
        map_y = np2lt::f32(params.prewarped_directory + "/map_y.npy");
    }

    int start = start_index;
    int end   = start_index + n_data_points;
    int index = 0;
    for (int i = start; i < end; i += batch_size) {
        std::cout << "Index: " << index << "\n";
        for (int j = 0; j < batch_size; ++j) {

            std::cout << "Loading: " << (i + j) << "\n";

            auto [d, l] = data[i + j];
            // Process the data

            auto li = l.item<int>();

            d = 255.0 - d;
            d = d.squeeze();

            // Pad the image if padding > 0
            if (padding > 0) {
                d = torch::nn::functional::pad(
                    d.unsqueeze(0).unsqueeze(0), // [1, 1, H, W]
                    torch::nn::functional::PadFuncOptions({padding, padding, padding, padding}).mode(torch::kConstant).value(255)
                ).squeeze(); // [H + 2*padding, W + 2*padding]
            }

            if (params.flip_input_H) {
                d = torch::fliplr(d);
            }

            if (params.flip_input_V) {
                d = torch::flipud(d);
            }

            if (!use_prewarped) {
                Image di = s4_Utils::TensorToImage(d);
                Texture ti = LoadTextureFromImage(di);
                SetTextureFilter(ti, TEXTURE_FILTER_BILINEAR);
                UnloadImage(di);

                batches[index / batch_size].textures.push_back(ti);
                batches[index / batch_size].labels.push_back(li);
            }
            else {
                // Prewarp the image using map_x and map_y
                // First convert d to float32
                d = d.to(torch::kFloat32).unsqueeze(0).unsqueeze(0); // [1, 1, H, W]

                // Resize it to 800x1280
                d = torch::nn::functional::interpolate(
                    d,
                    torch::nn::functional::InterpolateFuncOptions().size(std::vector<int64_t>({800, 1280})).mode(torch::kBilinear).align_corners(false)
                );

                // Apply remap
                d = torch::nn::functional::grid_sample(
                    d,
                    torch::stack({map_x, map_y}, -1).unsqueeze(0),
                    torch::nn::functional::GridSampleFuncOptions().mode(torch::kBilinear).padding_mode(torch::kBorder).align_corners(true)
                );

                // Convert back to uint8
                d = torch::clamp(d.squeeze() , 0, 255).to(torch::kUInt8); // [H, W]

                Image di = s4_Utils::TensorToImage(d);

                if ((i + j) == 0) {
                    ExportImage(di, "prewarped_sample.bmp");
                    std::cout << "INFO: [Helpers::Data::Get] Saved prewarped sample image to prewarped_sample.bmp\n";
                }

                Texture ti = LoadTextureFromImage(di);
                SetTextureFilter(ti, TEXTURE_FILTER_POINT);
                UnloadImage(di);

                std::cout << "INFO: [Helpers::Data::Get] Pushed to: batch " << index / batch_size << '\n';

                batches[index / batch_size].textures.push_back(ti);
                batches[index / batch_size].labels.push_back(li);
                
            }
        }
        index += batch_size;

    }

    return batches;


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
        2560,       /* Horizontal Resolution */
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
        2 * Width  * params.upscale_amount,   /* PEncoder Width */

        &opt,       /* Optimizer */

        params.process_fn  /* Processing function */

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

    scheduler.SetSubShaderThreshold(params.sub_shader_threshold);
    scheduler.SetBatchSize(params.n_batch_size);
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
        torch::Tensor action = eval_fn.sample(scheduler.maximum_number_of_frames_in_image);

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

    for (auto &batch : batches) {
        auto pp = Evaluate(params, scheduler, eval_fn, batch);
        perf.samples_total   += pp.samples_total;
        perf.samples_correct += pp.samples_correct;
        perf.entropy        += pp.entropy;;
        perf.loss           += pp.loss;
    }

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

    masks = np2lt::f32("source/Assets/regions2.npy");
    ratios = torch::ones({1, 10});

    loss_fn = [](torch::Tensor &preds, torch::Tensor &targets) -> torch::Tensor {
        
        return -torch::nn::functional::cross_entropy(
            preds,
            targets,
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        );
        

        // targets [1]
        // preds [1, 10]

        // Use MSE loss
        // note that each pred is summed over a 50x50 region
        // so we need to divide by (50*50) to get the average
        /*
        preds = preds / (50.0 * 50.0);

        auto target_one_hot = torch::nn::functional::one_hot(
            targets,
            preds.size(1)
        ).to(torch::kFloat32); // [1, 10]

        // MSE Loss
        auto loss = torch::nn::functional::mse_loss(
            preds,
            target_one_hot,
            torch::nn::functional::MSELossFuncOptions().reduction(torch::kNone)
        ); // [1, 10]

        // Return negative sum
        return -loss.sum(); // [1]
        */
    };

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

Helpers::_Result::_Result () {
    label = 0;
    prediction = 0;
    index = 0;
    batch_id = 0;
    reward = 0.0;
}

void Helpers::Parameters::ExportResults (const std::string &filename, int mode) {
    // Export as a CSV file
    std::ofstream ofs(filename);

    // The header is
    // index, label, prediction, correct

    ofs << "Index,Label,Prediction,Reward,Correct\n";

    auto _results = GetResults(mode);

    for (const auto &res : _results) {
        ofs << res.index << ','
            << res.label << ','
            << res.prediction << ','
            << res.reward << ','
            << (res.label == res.prediction ? 1 : 0) << '\n';
    }

    results.clear();
    ofs.close ();
    
}

std::vector<Helpers::_Result> Helpers::Parameters::GetResults (int mode) {
    std::vector<Helpers::_Result> output_results;

    int dataset_size = 0;
    int batch_size = 0;
    switch (mode) {
        case 0:
            dataset_size = n_training_samples; 
            batch_size   = n_batch_size;
            break;
        case 1:
            dataset_size = n_validation_samples;
            batch_size = n_validation_batch_size;
            break;
        case 2:
            dataset_size = n_test_samples;
            batch_size = n_test_batch_size;
            break;
        default:
            throw std::runtime_error("Runner::GetResults: Unsupported mode for getting results.");
    }

    // Within a batch, stride by 20
    // Between batches, stride by n_samples * 20

    // How many batches are there?
    int num_batches = dataset_size / batch_size;

    // What strides are required to advance to the next batch?
    int batch_stride = batch_size * n_samples * 20;

    // What strides within a stride is required to advance to the next sample in a batch?
    int sample_stride = 20;

    for (int b = 0; b < num_batches; ++b) {
        for (int i = 0; i < batch_size; ++i) {
            int index = b * batch_stride + i * sample_stride;
            if (index >= results.size()) {
                throw std::runtime_error("Runner::GetResults: Index out of bounds while getting results.");
            }
            const auto &res = results[index];
            output_results.push_back(res);
        }
    }

    return output_results;
}