

module system_t (
    input  wire clk,
    input  wire rst,
    input  wire vsync,
    input  wire serial,
    input  wire bitplane,
    output wire send
);

wire pvsync;
wire pbitplane;
wire pserial;
wire ssend;

// Sync
synchronizer_t# (
    .N_BIT_PLANES(12)
)
synch (
    .clk(clk),
    .rst(rst),
    .vsync(pvsync),
    .serial(pserial),
    .bitplane(pbitplane),
    .send(ssend)
);

// long pulse generation
long_t#(
    .DURATION(1200) // 12 MHz
) 
lsend (
    .clk(clk),
    .rst(rst),
    .in_pulse(ssend),
    .out_pulse(send)
);

// Pulse to clock
pulse_t vsync_0 (
  .clk(clk),
  .rst(rst),
  .in_pulse(vsync),
  .out_pulse(pvsync)
);

pulse_t serial_0 (
  .clk(clk),
  .rst(rst),
  .in_pulse(serial),
  .out_pulse(pserial)
);

pulse_t bitplane_0 (
  .clk(clk),
  .rst(rst),
  .in_pulse(bitplane),
  .out_pulse(pbitplane)
);



endmodule