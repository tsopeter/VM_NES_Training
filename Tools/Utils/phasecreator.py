import numpy as np

complex_types = [
    np.complex_,
    np.complex64,
    np.complex128
]

phase_levels = np.array([   \
    0.0000, 0.0100, 0.0205, 0.0422, 
    0.0560, 0.0727, 0.1131, 0.1734, 
    0.3426, 0.3707, 0.4228, 0.4916, 
    0.5994, 0.6671, 0.7970, 0.9375,
])

logical_masks = np.array([
    [
        [1, 0],
        [1, 0]
    ],
    [
        [1, 0],
        [0, 0]
    ],
    [
        [0, 0],
        [1, 0]
    ],
    [
        [1, 0],
        [1, 1]
    ],
    [
        [0, 0],
        [0, 0]
    ],
    [
        [1, 0],
        [0, 1]
    ],
    [
        [0, 0],
        [1, 1]
    ],
    [
        [0, 0],
        [0, 1]
    ],
    [
        [1, 1],
        [1, 0]
    ],
    [
        [1, 1],
        [0, 0]
    ],
    [
        [0, 1],
        [1, 0]
    ],
    [
        [0, 1],
        [0, 0]
    ],
    [
        [1, 1],
        [1, 1]
    ],
    [
        [1, 1],
        [0, 1]
    ],
    [
        [0, 1],
        [1, 1]
    ],
    [
        [0, 1],
        [0, 1]
    ]
], dtype=np.uint8)

def PLMQuantizerCore(Phase : np.ndarray, Lambda : float = 633e-9):
    Normalized_Phase = \
        (np.pi + (np.angle(Phase) if Phase.dtype in complex_types else Phase)) \
        / (2 * np.pi)
    Normalized_Phase_Flat = Normalized_Phase.flatten()

    # Update Normalized_Phase_Flat based on phase_levels
    mask_high = np.logical_and(Normalized_Phase_Flat > phase_levels[-1],
                                np.abs(Normalized_Phase_Flat - phase_levels[-1]) < np.abs(Normalized_Phase_Flat - 1))
    Normalized_Phase_Flat[mask_high] = phase_levels[-1]

    mask_low = np.logical_and(Normalized_Phase_Flat > phase_levels[-1],
                            np.abs(Normalized_Phase_Flat - phase_levels[-1]) >= np.abs(Normalized_Phase_Flat - 1))
    Normalized_Phase_Flat[mask_low] = phase_levels[0]

    # Reshape phase_levels for broadcasting
    phase_levels_reshaped = phase_levels.reshape(1, -1)  # Shape (1, num_levels)
    # Calculate the absolute differences
    differences = np.abs(Normalized_Phase_Flat[:, np.newaxis] - phase_levels_reshaped)  # Shape (num_elements, num_levels)
    # Get indices of the minimum differences
    closest_indices : np.ndarray = np.argmin(differences, axis=1)
    Quantized = closest_indices.reshape(Phase.shape)

    # Get logical mask
    Masks   = logical_masks[Quantized]
    Logical = Masks.transpose(0, 2, 1, 3).reshape(2*Phase.shape[0], 2*Phase.shape[1])

    return Quantized, Logical

def PLMQuantizer(Phase  : np.ndarray,
                 Lambda : float = 633e-9,
                 mode   : str   = "Normal"):
    '''
        PLMQuantizer takes in a mxn image and
        returns a 2mx2n image where each 4-pixels of
        the resultant corresponds to a singluar phase
        value

        Phase:  np.ndarray | Real | Complex
            Phase mask to be quantized.
            If Phase is Real, then it is taken as
            is.
            If Phase is Complex, then it will be taken
            as np.angle(Phase)    

        Lambda : float
            Lambda is the wavelength of the input light.
        
        mode : str
            "Test"  If mode is set to test, we return in-addtion
            three other masks:

                Quantized, Logical, and Flipped
    '''
    # Run core
    Quantized, Logical = PLMQuantizerCore(Phase, Lambda)


    # return values based on mode
    match mode:
        case "Test" | "test" | "t" | "T":
            return phase_levels[Quantized], Logical, np.fliplr(Logical)
        case _:
            return np.fliplr(Logical)
        
def PLMQuantizerGenerateMask (Phase : np.ndarray, Lambda : float = 633e-9)->np.ndarray:
    Quantized = PLMQuantizer(Phase, Lambda, mode="Nill") * 255
    return np.dstack([Quantized, Quantized, Quantized])

if __name__ == '__main__':
    from scipy.io import loadmat
    from matplotlib import pyplot as plt
    Phase      = np.load('./Phase_Masks/2024-10-01-20-55-07-705930.npy')
    Quantized, Logical, Flipped  = PLMQuantizer(Phase, mode="Test")

    MQuantized = loadmat('./Test_Files/Quantized_Phase.mat')['pslm_phaseq']
    MLogical   = loadmat('./Test_Files/Logical.mat')['pslm_logical']
    MFLipped   = loadmat('./Test_Files/Flipped.mat')['pslm_flip']

    # test
    print(f'Difference: Quantization {np.sum(np.abs(Quantized - MQuantized))},\tdtype={Quantized.dtype}')
    print(f'Difference: Logical      {np.sum(np.abs(Logical   - MLogical  ))},\tdtype={Logical.dtype}')
    print(f'Difference: Flipped      {np.sum(np.abs(Flipped   - MFLipped  ))},\tdtype={Flipped.dtype}')












