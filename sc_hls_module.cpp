#include "systemc.h"
SC_MODULE(hls_sc_module)
{
    sc_in<bool> clk;
    sc_in<bool> data;
    sc_inout<bool> regc,regd;
    void reg_proc()
    {
        regc.write(data.read());
        regd.write(regc.read());
    }
    SC_CTOR(hls_sc_module)
    {
        SC_METHOD(reg_proc);
        sensitive_pos << clk;
    }
};

SC_MODULE(hls_sc_module)
{
    sc_in<bool> clk;
    sc_in<bool> data;
    sc_inout<bool> rega,regb;
    
    bool rega_v,regb_v;
    void reg_proc()
    {
        rega_v = data.read;
        regb_v = rega_v;
        rega.write(rega_v);
        regb.write(regb_v);
    }
    SC_CTOR(hls_sc_module)
    {
        SC_METHOD(reg_proc);
        sensitive_pos << clk;
    }
}

#include "systemc.h"
SC_MODULE(count_zeros_comb)
{
    sc_in<sc_uint<8> > in;
    sc_out<sc_uint<8> > out;
    sc_out<bool> error;
    bool valid(<sc_uint<8> x);
    sc_uint<4> zeros(sc_uint<8> x);
    void control_proc();

    SC_CTOR(count_zeros_comb)
    {
        SC_METHOD(control_proc);
        sensitive << in;
    }
};

#include "count_zeros_comb.h"
void count_zeros_comb::control_proc()
{
    sc_uint<4> tmp_out;
    bool is_valid = valid(in.read());
    error.write(! is_valid);
    is_valid? tmp_out = zeros(in.read()): tmp_out = 0;   
    out.write(tmp_out);
}

bool count_zeros_comb::valid(sc_uint<8> x)
{
    bool is_valid = 1;
    bool seen_zero = 0;
    bool seen_trailing = 0;
    for(int i=0; i<8;i++)
    {
        if(seen_trailing && x[i]==0)
        {
            is_valid = 0;
            break;
        }
        else if(seen_zero && x[i]==1)
            seen_trailing = 1;
        else if(x[i]==0)
            seen_zero = 1;

    }
    return is_valid;
}

sc_uint<4> count_zeros_comb::zeros(sc_uint<8> x)
{
    int count = 0;
    for(int i=0;i<8;i++)
    {
        if(x[i] == 0)
            count ++;
    }
    return count;
}


////////////
#include "systemc.h"
#define ZEROS_WIDTH 4
#define MAX_BIT_READ 7
SC_MODULE(count_zeros_seq)
{
    sc_in<bool> clk,reset,read, data;
    sc_out<bool> is_legal,data_ready;
    sc_out<sc_uint<ZEROS_WIDTH> > zeros;

    sc_signal<bool> is_legal_reg_s, data_ready_reg_s,
    seen_trailing_reg_s, seen_zero_reg_s,
    next_is_legal_s, next_data_ready_s,
    next_seen_trailing_s, next_seen_zero_s;

    sc_signal<sc_uint<ZEROS_WIDTH> > zeros_reg_s,next_zeros_s;
    sc_signal<sc_uint<ZEROS_WIDTH-1> > bits_seen_reg_s,next_bits_seen_s;

    //processes
    void comb_logic();
    void seq_logic();
    void output_logic();
    //functions
    void set_defaults();

    SC_CTOR(count_zeros_seq)
    {
        SC_METHOD(comb_logic);
        sensitive << data << read << is_legal_reg_s << data_ready_reg_s;
        sensitive << seen_trailing_reg_s << seen_zero_reg_s << zeros_reg_s << bits_seen_reg_s;

        SC_METHOD(seq_logic);
        sensitive_pos << clk << reset;

        SC_METHOD(output_logic);
        sensitive << is_legal_s << data_ready_s << zeros_s;

    }
}

#include "count_zeros_seq.h"
//find a singular run of zeros
void count_zeros_seq::comb_logic()
{
    set_defaults();
    if(read.read())
    {
        if(seen_trailing_reg_s && data.read()==0)
        {
            next_is_legal_s = false;
            next_zeros_s = 0;
            next_data_ready_s = true;
        }
        else if(seen_zero_reg_s && data.read() == 1)
        {
            next_seen_zero_s = true;
        }
        else if(data.read() == 0)
        {
            next_seen_zero_s = true;
            next_zeros_s = zeros_reg_s.read() + 1;
        }

        if(bits_seen_reg_s.read() == MAX_BIT_READ)
            next_data_ready_s = true;
        else
            next_bits_seen_s = bits_seen_reg_s.read() + 1;
    }
}
void count_zeros_seq::seq_logic()
{
    if(reset)
    {
        zeros_reg_s = 0;
        bits_seen_reg_s = 0;
        seen_zero_reg_s = 0;
        seen_trailing_reg_s = 0;
        is_legal_reg_s = 0;
        data_ready_reg_s = 0;
    }
    else
    {
        zeros_reg_s = next_zeros_s;
        bits_seen_reg_s = next_bits_seen_s;
        seen_zero_reg_s = next_seen_zero_s;
        seen_trailing_reg_s = next_seen_trailing_s;
        is_legal_reg_s = next_is_legal_s;
        data_ready_reg_s = next_data_ready_s;
    }
}