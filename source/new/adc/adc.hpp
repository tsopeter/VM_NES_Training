#ifndef adc_hpp__
#define adc_hpp__

#include <cstdint>

// Register addresses
#define HEARTBEAT         0x43c00000
#define ID                0x43c00004

#define ADC_RESET         0x43c00010
#define ADC_BURST_LIMIT   0x43c00014
#define ADC_START         0x43c00018
#define ADC_START_PERSIST 0x43c0001C

#define CD2_PERIOD        0x43c00020
#define CD2_WIDTH         0x43c00024

#define ADC_TOTAL_PULSES  0x43c00108
#define ADC_FRAME_COUNT   0x43c00200
#define ADC_TOTAL_WRITES  0x43c00208

#define ADC_N_AVERAGE     0x43c00210
#define ADC_DELAY         0x43c00214
#define ADC_ENABLE_MEAN   0x43c00218

#define DAC_RESET         0x43c00030
#define DAC_N_LEVELS      0x43c00034
#define DAC_SWAP_TRIGGER  0x43c00038

#define DEBOUNCE_LIMIT    0x43c00048

#define IRQ_ENABLE        0x43c00400
#define MEM_BUF_SEL       0x43c00404
#define MEM_MODES         0x43c00408
#define IRQ_FRAME_CLEAR   0x43c0040C
#define ADC_FRAME         0x43c00410
#define ADC_FRAME2        0x43c00420

#define RAM_BASE_ADDR     0x43c01000
#define RAM_SIZE          0x1000

class ADC {
public:
    ADC ();
    ~ADC();

    void     Write(uint32_t reg_addr, uint32_t value);
    uint32_t Read(uint32_t reg_addr);
    uint32_t Read_Memory(uint32_t offset);

private:
    void Map_Memory ();
    void Unmap_Memory ();
    void Write_Register(uint32_t reg_addr, uint32_t value);
    uint32_t Read_Register(uint32_t reg_addr);

    // Memory mapped I/O
    int m_mem_fd{-1};
    void * m_mapped_base{nullptr};
    volatile uint32_t * m_registers{nullptr};
};


#endif
