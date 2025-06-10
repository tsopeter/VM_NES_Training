

module long_t #(
    parameter DURATION = 16  // Number of clock cycles for output pulse
)(
    input  wire clk,
    input  wire rst,
    input  wire in_pulse,     // Single-cycle input pulse
    output reg  out_pulse     // Extended output pulse
);

    reg [$clog2(DURATION)-1:0] counter;
    reg active;

    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            counter    <= '0;
            out_pulse  <= 1'b0;
            active     <= 1'b0;
        end else begin
            if (in_pulse && !active) begin
                active    <= 1'b1;
                counter   <= DURATION - 1;
                out_pulse <= 1'b1;
            end else if (active) begin
                if (counter == 0) begin
                    active    <= 1'b0;
                    out_pulse <= 1'b0;
                end else begin
                    counter <= counter - 1;
                end
            end
        end
    end

endmodule