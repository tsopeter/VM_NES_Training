# Performance Metrics

- Post-Training Quantization (PTQ)
    - 1000 Train/1000 Valid/1000 Test
        - (1) (silico.ptq.png)
            - Train Physical (62.3) Simulation (62.2)
            - Valid Physical (54.1) Simulation (57.5)
            - Test  Physical (57.8) Simulation (56.6)
        - (2) (silico.ptq.3.png)
            - Train Physical (64.8) Simulation (68.9)
            - Valid Physical (59.8) Simulation (63.4)
            - Test  Physical (59.3) Simulation (62.3)
        - (3) (silico.ptq.4.png)
            - Train Physical (66.30) Simulation (73.80)
            - Valid Physical (58.50) Simulation (66.20)
            - Test  Physical (60.80) Simulation (67.10)
    - 10000 Train/5000 Valid/5000 Test
        - (1) (silico.ptq.6.png)
            - Train Physical (65.77) Simulation (72.95)
            - Valid Physical (66.64) Simulation (71.78)
            - Test  Physical (63.38) Simulation (70.36)
        - (2) (silico.ptq.8.png)
            - 
        


- Quantization-Aware Training (QAT)
    - Straight-Through Estimator (STE)
        - 1000 Train/1000 Valid/1000 Test
            - (1) (silico.ste.2.png)
                - Train Physical (61.1) Simulation (63.5)
                - Valid Physical (53.3) Simulation (58.1)
                - Test  Physical (59.1) Simulation (59.2)
            - (2) (silico.ste.3.png)
                - Train Physical (64.8) Simulation (67.5)
                - Valid Physical (59.4) Simulation (62.8)
                - Test  Physical (61.0) Simulation (61.2)
            - (3) (silico.ste.4.png)
                - Train Physical (70.20) Simulation (73.60)
                - Valid Physical (60.40) Simulation (67.10)
                - Test  Physical (61.60) Simulation (67.10)
        - 10000 Train/5000 Valid/5000 Test
            - (1) (silico.ste.6.png)
                - Train Physical (64.94) Simulation (72.78)
                - Valid Physical (66.02) Simulation (72.12)
                - Test  Physical (63.02) Simulation (70.12)


- Gradient-Free Training (In-Silico)
    - Categorical
        - 1000 Train/1000 Valid/1000 Test
            - Train Physical () Simulation ()
            - Valid Physical () Simulation ()
            - Test  Physical () Simulation ()
        
    - Normal
        - 1000 Train/1000 Valid/1000 Test
            - Train Physical () Simulation ()
            - Valid Physical () Simulation ()
            - Test  Physical () Simulation ()
        
- Gradient-Free Training (In-Situ)
    - Categorical
        - 1000 Train/1000 Valid/1000 Test
            - Train Physical (80.1)
            - Valid Physical (70.3)
            - Test  Physical (68.4)

    - Normal 
        - 1000 Train/1000 Valid/1000 Test
            - Train Physical ()
            - Valid Physical ()
            - Test  Physical ()


For a single layer diffractive model, there is little to no difference between post-training-quantization (PTQ) and quantization-aware-training (QAT).

## What question are we answering?
How does the non-uniform quantization on the Phase Light Modulator affect training of machine learning models?

The non-uniform quantization can affect how models train under two different methods: gradient-based methods and gradient-free methods. We show that gradient-based methods under a shallow model are not perceptable to quantization effects, while gradient-free models are affected by it.






## Quantization Effects caused by Non-Uniform Step Size

Unlike spatial light modulators (SLMs), the DMD-based DLP has non-uniform step size, where phases closer to 2 pi exhibit much larger steps than those closer to 0. We show that the non-uniform step sizes cause little to no error. However, initialization of parameters is important 

We show that a single layer model is not affected by quantization effects, and tha

#### Model Information

The model uses 



#### Physical Testbed


#### Results


### Gradient Free Optimization

Gradient-Free Optimization treats the system as a non-differentiable black-box which cannot use backpropagation to update the weights. Most systems use gradient-free methods such as Natural Evolution Strategies to update the weights. These methods require many samples to estimate the gradient. However, most utilize Gaussian Distribution for phase manipulation. For a given varaince, it may under explore large step sizes and under explore small step sizes. 

We show that the distribution itself can affect the rate of convergence of the model, and that Gaussian distributions are not prefered. 

Treating the parameters not as phase components, but probabilities and utilizing Categorical distribution to ensure equal distance between levels produces better performance compared to a Gaussian distribution with fixed standard deviation. 






