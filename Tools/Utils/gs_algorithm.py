import numpy as np
def gs_algorithm(Image : np.array, iterations : int = 50)->np.array:
    Target = np.sqrt(Image)
    Object = np.exp(1j*np.random.rand(Target.shape[0], Target.shape[1])*2*np.pi)

    phase = lambda p : np.divide(p, np.abs(p))

    for _ in range(0, iterations):
        U      = np.fft.ifft2(np.fft.ifftshift(Object))
        Up     = np.multiply(Target, phase(U))
        D      = np.fft.fft2(np.fft.fftshift(Up))
        Object = phase(D)

    return Object