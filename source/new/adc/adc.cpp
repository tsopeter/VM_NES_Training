
#include "adc.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

ADC::ADC () {
    Map_Memory();
    Write_Register(ADC_BURST_LIMIT, 24);
    Write_Register(IRQ_ENABLE, 1);  // Enable interrupts in hardware
    Write_Register(MEM_MODES, 1);   // Ping pong buffer
    Write_Register(ADC_START_PERSIST, 1); // Start ADC in persistent mode (continuous acquisition)
}

ADC::~ADC() {
    Unmap_Memory();
}

void ADC::Write(uint32_t reg_addr, uint32_t value) {
    Write_Register(reg_addr, value);
}

uint32_t ADC::Read(uint32_t reg_addr) {
    return Read_Register(reg_addr);
}


void ADC::Map_Memory() {
    // Open /dev/mem for memory-mapped I/O access
    m_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (m_mem_fd < 0) {
        std::cerr << "[APP CLIENT] Error opening /dev/mem: " << strerror(errno) << std::endl;
        std::cerr << "[APP CLIENT] Running without hardware access (simulation mode)" << std::endl;
        
        // Create dummy registers for testing
        size_t map_size = RAM_BASE_ADDR + RAM_SIZE - HEARTBEAT;
        std::cout << "[APP CLIENT] Allocating " << map_size << " bytes for simulation registers" << std::endl;
        m_mapped_base = calloc(1, map_size);
        if (m_mapped_base) {
            m_registers = static_cast<volatile uint32_t*>(m_mapped_base);
            std::cout << "[APP CLIENT] Allocated " << map_size << " bytes for simulation registers" << std::endl;
        }

        // Zero out the registers to start with a clean slate
        if (m_registers) {
            size_t num_registers = map_size / sizeof(uint32_t);
            std::cout << "[APP CLIENT] Initializing " << num_registers << " simulation registers to 0" << std::endl;
            for (size_t i = 0; i < num_registers; ++i) {
                m_registers[i] = 0;
            }
        }
        return;
    }

    // Map the register space
    size_t map_size = RAM_BASE_ADDR + RAM_SIZE - HEARTBEAT;
    m_mapped_base = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_mem_fd, HEARTBEAT);
    
    if (m_mapped_base == MAP_FAILED) {
        std::cerr << "[APP CLIENT] Error mapping memory: " << strerror(errno) << std::endl;
        close(m_mem_fd);
        m_mem_fd = -1;
        
        // Create dummy registers for testing
        m_mapped_base = calloc(1, map_size);
        if (m_mapped_base) {
            m_registers = static_cast<volatile uint32_t*>(m_mapped_base);
            std::cout << "[APP CLIENT] Allocated " << map_size << " bytes for simulation registers" << std::endl;
        }
        return;
    }

    m_registers = static_cast<volatile uint32_t*>(m_mapped_base);
    std::cout << "[APP CLIENT] Memory mapped successfully" << std::endl;
}

void ADC::Unmap_Memory() {
    if (m_mapped_base && m_mapped_base != MAP_FAILED) {
        if (m_mem_fd >= 0) {
            // Real hardware mapping - use munmap
            size_t map_size = RAM_BASE_ADDR + RAM_SIZE - HEARTBEAT;
            munmap(m_mapped_base, map_size);
        } else {
            // Simulation mode - use free
            free(m_mapped_base);
        }
        m_mapped_base = nullptr;
        m_registers = nullptr;
    }
    
    if (m_mem_fd >= 0) {
        close(m_mem_fd);
        m_mem_fd = -1;
    }
}

void ADC::Write_Register(uint32_t reg_addr, uint32_t value) {
    if (!m_registers) {
        std::cerr << "[APP CLIENT] Write Error: registers not mapped @ address 0x" << std::hex << reg_addr << std::dec << std::endl;
        return;
    }
    
    uint32_t offset = (reg_addr - HEARTBEAT) / 4;
    m_registers[offset] = value;
    
    if (m_mem_fd < 0) {
        // Simulation mode - show what we're doing
        std::cout << "[APP CLIENT] [SIM] Write 0x" << std::hex << reg_addr 
                  << " = 0x" << value << std::dec << std::endl;
    }
}

uint32_t ADC::Read_Register(uint32_t reg_addr) {
    if (!m_registers) {
        std::cerr << "[APP CLIENT] Read Error: registers not mapped @ address 0x" << std::hex << reg_addr << std::dec << std::endl;
        return 0xDEADBEEF;  // Dummy value for error case
    }
    
    uint32_t offset = (reg_addr - HEARTBEAT) / 4;
    uint32_t value = m_registers[offset];
    
    if (m_mem_fd < 0) {
        // Simulation mode - show what we're doing
        std::cout << "[APP CLIENT] [SIM] Read 0x" << std::hex << reg_addr 
                  << " = 0x" << value << std::dec << std::endl;
    }
    
    return value;
}

uint32_t ADC::Read_Memory(uint32_t offset) {
    return Read_Register(RAM_BASE_ADDR + offset * 4);
}