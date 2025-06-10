
module base (
    input  wire clk,

    input  wire vsync,
    input  wire bitplane,
    output wire send
);

//
// Comes from ZyBO serial interface
wire serial;

//
// System
system_t sys (
    .clk(clk),
    .rst(rst),
    .vsync(vsync),
    .serial(serial),
    .bitplane(bitplane),
    .send(Send)
);

always @(posedge clk) begin
    
end






endmodule