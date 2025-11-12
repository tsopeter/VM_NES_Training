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

