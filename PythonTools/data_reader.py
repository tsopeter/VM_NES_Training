import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

def read_file(filename : str):
    # Attempt to use read_data first
    try:
        data = read_data (filename)
    except:
        print(f'Fallback to legacy method for reading data from {filename}')
        data = read_file_old (filename)
    return data

# Read the data from file
def read_data(file_path):
    results_path = f'{file_path}/train/a/results.csv'
    info_path    = f'{file_path}/train/a/mean_reward.txt'

    f = open(info_path, "r")

    # Skip to Model Parameters
    for _ in range (13):
        f.readline ()

    # Extract info
    Height = int (f.readline ().split (':')[1].strip ())
    Width  = int (f.readline ().split (':')[1].strip ())
    Dist   = f.readline ().split (':')[1].strip ()

    if Dist == 'normal':
        Std = float (f.readline ().split (':')[1].strip ())
    else:
        Std = 0.0

    # Skip Upscale Factor
    f.readline ()

    Quant = int (f.readline ().split (':')[1].strip ())

    # Skip to Training Parameters
    f.readline ()

    Lr = float (f.readline ().split (':')[1].strip ())
    f.readline ()  # Optimizer
    SamplesPerImage = int (f.readline ().split (':')[1].strip ())
    Epochs = int (f.readline ().split (':')[1].strip ())

    # Skip SUR and SUA
    for _ in range (2):
        f.readline ()

    # Close the file
    f.close ()

    info = {
        'Height': Height,
        'Width': Width,
        'Dist': Dist,
        'Std': Std,
        'Quant': Quant,
        'Lr': Lr,
        'SamplesPerImage': SamplesPerImage,
        'Epochs': Epochs,
        'Training_Data' : None,
        'Validation_Data' : None,
        'Test_Data' : None,
        'Training_Results' : None,
        'Validation_Results' : None,
        'Test_Results' : None
    }

    data = pd.read_csv(results_path)

    # Split data into Training, Validation, and Test.
    info['Training_Data'] = data[data['DataType'] == 'Training']
    info['Validation_Data'] = data[data['DataType'] == 'Validation']
    info['Test_Data'] = data[data['DataType'] == 'Test']

    # Read the csv files
    train_csv = f'{file_path}/train/epoch_{Epochs - 1}/training_inference_results.csv'
    val_csv   = f'{file_path}/train/epoch_{Epochs - 1}/validation_results.csv'
    test_csv  = f'{file_path}/train/test_results.csv'

    info['Training_Results'] = pd.read_csv(train_csv)
    info['Validation_Results'] = pd.read_csv(val_csv)
    info['Test_Results'] = pd.read_csv(test_csv)

    return info

def read_file_old (filename : str, mode : int= 0):
    # We know the the return is structured as
    # {epoch      : np.array,
    #  train_loss : np.array,
    #  val_loss   : np.array,
    #  train_acc  : np.array,
    #  val_acc    : np.array,
    #  test_loss  : np.array,
    #  test_acc   : np.array,
    #  test_conf  : np.array,
    #  dist       : str,
    #  lr         : float,
    #  std        : float,
    #  quant      : int,
    #  Height     : int,
    #  Width      : int,
    #  Epochs     : int,
    #  samples_per_image : int
    # }

    '''
     File structure:
    
    Dataset parameters:
        Data Padding Amount: <int>
        Sub Shader Threshold: <float>
        Training Dataset:
            Training Size: <int>
            Training Batch Size: <int>
        Validation Dataset:
            Validation Size: <int>
            Validation Batch Size: <int>
        Test Dataset:
            Test Size: <int>
            Test Batch Size: <int>
    Model Parameters:
        Model Height: <int>
        Model Width: <int>
        Model Distribution: <str>
            Standard Deviation: <float> (If normal)
        Upscale Factor: <int>
        Number of Levels: <int>
    Training Parameters:
        Learning Rate: <float>
        Optimizer: <str>
        Samples per Image: <int>
        Epochs: <int>
        Sample Update Rate: <int>
        Sample Update Amount: <int>
    ----------------------------------------
    <Dataset Type>
    Epoch <int>
    Time: <hr:min:sec>
    Epoch: <int>
    Compute Time (s): <int>
    Samples Total: <int>
    Samples Correct: <int>
    Accuracy: <float>
    Entropy: <float>
    Loss: <float>
    ----------------------------------------
    ...
    '''
    data = {}
    file_path = filename
    filename = filename + '/a/mean_reward.txt'

    with open (filename, 'r') as f:
        # Skip to Model Parameters
        for _ in range (13):
            f.readline ()

        # Extract info
        Height = int (f.readline ().split (':')[1].strip ())
        Width  = int (f.readline ().split (':')[1].strip ())
        Dist   = f.readline ().split (':')[1].strip ()

        if Dist == 'normal':
            Std = float (f.readline ().split (':')[1].strip ())
        else:
            Std = 0.0

        # Skip Upscale Factor
        f.readline ()

        Quant = int (f.readline ().split (':')[1].strip ())

        # Skip to Training Parameters
        f.readline ()

        Lr = float (f.readline ().split (':')[1].strip ())
        f.readline ()  # Optimizer
        SamplesPerImage = int (f.readline ().split (':')[1].strip ())
        Epochs = int (f.readline ().split (':')[1].strip ())

        # Skip SUR and SUA
        for _ in range (2):
            f.readline ()

        # Prepare data arrays
        data['Height'] = Height
        data['Width']  = Width
        data['dist']   = Dist
        data['std']    = Std
        data['quant']  = Quant
        data['lr']     = Lr
        data['epochs'] = Epochs
        data['samples_per_image'] = SamplesPerImage
        data['Training_Data'] = {
            'Epoch' : np.arange(Epochs),
            'CurrentTime' : np.zeros(Epochs),
            'ComputeTime' : np.zeros(Epochs),
            'SamplesTotal' : np.zeros(Epochs),
            'SamplesCorrect' : np.zeros(Epochs),
            'Entropy' : np.zeros(Epochs),
            'Accuracy' : np.zeros(Epochs),
            'Loss' : np.zeros(Epochs)
        }
        data['Validation_Data'] = {
            'Epoch' : np.arange(Epochs),
            'CurrentTime' : np.zeros(Epochs),
            'ComputeTime' : np.zeros(Epochs),
            'SamplesTotal' : np.zeros(Epochs),
            'SamplesCorrect' : np.zeros(Epochs),
            'Entropy' : np.zeros(Epochs),
            'Accuracy' : np.zeros(Epochs),
            'Loss' : np.zeros(Epochs)
        }
        data['Test_Data'] = {
            'Epoch' : np.array([Epochs]),
            'CurrentTime' : 0.0,
            'ComputeTime' : 0.0,
            'SamplesTotal' : 0,
            'SamplesCorrect' : 0,
            'Entropy' : np.zeros(1),
            'Accuracy' : np.zeros(1),
            'Loss' : np.zeros(1)
        }
        data['Training_Results'] = None
        data['Validation_Results'] = None
        data['Test_Results'] = None

        try:
            while True:
                # Skip separator
                f.readline ()

                # Read dataset type
                dataset_type = f.readline ().strip ()

                if dataset_type == 'Final Testing Results':
                    f.readline ()  # Skip Epoch
                    f.readline ()  # Skip Epoch (redundant)
                    f.readline ()  # Skip compute time
                    f.readline ()  # Skip samples total
                    f.readline ()  # Skip samples correct
                    test_acc = float (f.readline ().split (':')[1].strip ())
                    test_entropy = float (f.readline ().split (':')[1].strip ())
                    test_loss = float (f.readline ().split (':')[1].strip ())

                    data['Test_Data']['Accuracy'][0] = test_acc
                    data['Test_Data']['Entropy'][0] = test_entropy
                    data['Test_Data']['Loss'][0] = test_loss
                    break

                # Skip Epoch
                f.readline ()

                # Skip Time
                f.readline ()

                # Skip Epoch (redundant)
                epoch = int (f.readline ().split (':')[1].strip ())
                if epoch >= Epochs:
                    break

                # Skip compute time
                f.readline ()

                # Skip samples total and correct
                for _ in range (2):
                    f.readline ()

                acc = float (f.readline ().split (':')[1].strip ())
                entropy = float (f.readline ().split (':')[1].strip ())
                loss = float (f.readline ().split (':')[1].strip ())

                if dataset_type == 'Training Inference':
                    # For simplicity, we use same data for train and val
                    data['Training_Data']['Accuracy'][epoch] = acc
                    data['Training_Data']['Entropy'][epoch] = entropy
                    data['Training_Data']['Loss'][epoch] = loss
                elif dataset_type == 'Validation':
                    data['Validation_Data']['Accuracy'][epoch] = acc
                    data['Validation_Data']['Entropy'][epoch] = entropy
                    data['Validation_Data']['Loss'][epoch] = loss

                # Skip separator
                f.readline ()
        except:
            pass

    # Turn Training Data, Validation Data into pandas DataFrames
    data['Training_Data'] = pd.DataFrame(data['Training_Data'])
    data['Validation_Data'] = pd.DataFrame(data['Validation_Data'])
    data['Test_Data'] = pd.DataFrame(data['Test_Data'])

    # Read the csv files
    train_csv = f'{file_path}/epoch_{Epochs - 1}/training_inference_results.csv'
    val_csv   = f'{file_path}/epoch_{Epochs - 1}/validation_results.csv'
    test_csv  = f'{file_path}/test_results.csv'

    data['Training_Results'] = pd.read_csv(train_csv)
    data['Validation_Results'] = pd.read_csv(val_csv)
    data['Test_Results'] = pd.read_csv(test_csv)

    return data
  