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
};

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
void count_zeros_seq::set_defaults()
{
    next_zeros_s = zeros_reg_s;
    next_bits_seen_s = bits_seen_reg_s;
    next_seen_zero_s =  seen_zero_reg_s;
    next_seen_trailing_s =  seen_trailing_reg_s;
    next_is_legal_s = is_legal_reg_s;
    next_data_ready_s = data_ready_reg_s;
}
////fir controller and datapath
#include <systemc.h>
#include "fir_datapath.h"
#include "fir_fsm.h"

SC_MODULE(fir_top)
{
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_in<bool> in_valid;
    sc_in<int> sample;
    sc_out<bool> data_ready;
    sc_out<int> result;

    sc_signal<unsigned> state_out_s;

    fir_fsm *fir_fsm1;
    fir_datapath *fir_datapath1;

    SC_CTOR(fir_top)
    {
        fir_fsm1 = new fir_fsm("fir_fsm_inst");
        fir_fsm1->clock(clk);
        fir_fsm1->reset(reset);
        fir_fsm1->in_valid(in_valid);
        fir_fsm1->state_out(state_out);
        
        fir_datapath1 = new fir_datapath("fir_datapath_inst");
        fir_datapath1->clock(clk);
        fir_datapath1->sample(sample);
        fir_datapath1->state_out(state_out);
        fir_datapath1->result(result);
        fir_datapath1->data_ready(data_ready);
    }
};
//controller fsm
SC_MODULE(fir_fsm)
{
    sc_in<bool> clock;
    sc_in<bool> reset;
    sc_in<bool> in_valid;
    sc_out<unsigned> state_out;

    enum {reset_s, first_s,second_s,third_s,output_s, wait_s} state;

    void entry();

    SC_CTOR(fir_fsm)
    {
        SC_METHOD(entry);
        sensitive_pos << clock;
    }
};
//
#include "systemc.h"
#include "fir_fsm.h"

void fir_fsm::entry()
{
    sc_uint<3> state_tmp;
    if(reset.read())
    {
        state = reset_s;
    }
    switch(state)
    {
    case reset_s:
        state = wait_s;
        state_tmp = 0;
        state_out.write(state_tmp);
        break;
    case first_s:
        state = second_s;
        state_tmp = 1;
        state_out.write(state_tmp);
        break;
    case second_s:
        state = third_s;
        state_tmp = 2;
        state_out.write(state_tmp);
        break;
    case third_s:
        state = output_s;
        state_tmp = 3;
        state_out.write(state_tmp);
        break;
    case output_s:
        state = wait_s;
        state_tmp = 4;
        state_out.write(state_tmp);
        break;
    default:
        if(in_valid.read() == true)
            state = first_s;
        state_tmp = 5;
        state_out.write(state_tmp);
        break;

    }
}

//datapath
SC_MODULE(fir_datapath)
{
    sc_in<unsigned> state_out;
    sc_in<int> clock;
    sc_in<int> sample;
    sc_out<int> result;
    sc_out<bool> data_ready;

    sc_int<19> acc;
    sc_int<8> shift[16];
    sc_int<9> coefs[16];
    SC_CTOR(fir_datapath)
    {
        SC_METHOD(entry);
        sensitive_pos << clock;
    }
    void entry();
};

#include <systemc.h>
#include "fir_datapath.h"
void fir_datapath::entry()
{
    #include "fir_const_rtl.h"
    sc_int<8> sample_tmp;
    sc_uint<8> state = state_out.read();
    switch(state){
    case 0:
        sample_tmp = 0;
        acc = 0;
        for(int i=0;i<16;i++)
            shift[i]=0;
        result.write(0);
        data_ready.write(false);
    case 1:
        sample_tmp = sample.read();
        acc = sample_tmp*coefs[0];
        acc += shift[14]*coefs[15];
        acc += shift[13]*coefs[14];
        acc += shift[12]*coefs[13];
        acc += shift[11]*coefs[12];
        data_ready.write(false);
        break;
    case 2:
        acc += shift[10]*coefs[11];
        acc += shift[9]*coefs[10];
        acc += shift[8]*coefs[9];
        acc += shift[7]*coefs[8];
        data_ready.write(false);
        break;
    case 3:
        acc += shift[6]*coefs[7];
        acc += shift[5]*coefs[6];
        acc += shift[4]*coefs[5];
        acc += shift[3]*coefs[4];
        data_ready.write(false);
        break;
    case 4:
        acc += shift[2]*coefs[3];
        acc += shift[1]*coefs[2];
        acc += shift[0]*coefs[1];
        for(int i=14;i>=0;i--)
            shift[i+1] = shift[i];
        shift[0] = sample.read();
        data_ready.write(true);
        result.write(acc);
        break;
    case 5:
        data_ready.write(false);
        result.write(0);
    }
}