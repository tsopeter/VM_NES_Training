
module pulse_t (
    input  wire clk,
    input  wire rst,
    input  wire in_pulse,     // Input pulse (can be longer than one clock)
    output wire out_pulse     // Output pulse (one clock cycle)
);

    wire prev_in;

    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            prev_in    <= 1'b0;
            out_pulse  <= 1'b0;
        end else begin
            out_pulse  <= in_pulse & ~prev_in;  // Rising edge detection
            prev_in    <= in_pulse;
        end
    end

endmodule