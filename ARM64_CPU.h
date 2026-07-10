#pragma once
#include <cstdint>
#include <boost/multiprecision/cpp_int.hpp>
#include<iostream>
#include "memory.h"
using namespace boost::multiprecision;

enum Reg {
	// ======================
	// W registers (0 - 31)
	// ======================
	W0 = 0, W1, W2, W3,
	W4, W5, W6, W7,
	W8, W9, W10, W11,
	W12, W13, W14, W15,
	W16, W17, W18, W19,
	W20, W21, W22, W23,
	W24, W25, W26, W27,
	W28, W29, W30, W31,

	// ======================
	// X registers (32 - 63)
	// ======================
	X0 = 32, X1, X2, X3,
	X4, X5, X6, X7,
	X8, X9, X10, X11,
	X12, X13, X14, X15,
	X16, X17, X18, X19,
	X20, X21, X22, X23,
	X24, X25, X26, X27,
	X28, X29, X30, X31,

};

enum ShiftType {
	LSL,
	LSR,
	ASR,
	ROR
};

enum Extend {
	UXTB,
	UXTH,
	UXTW,
	UXTX,
	SXTB,
	SXTH,
	SXTW,
	SXTX
};

enum Cond {
	EQ,
	NE,
	CS,
	CC,
	MI,
	PL,
	VS,
	VC,
	HI,
	LS,
	GE,
	LT,
	GT,
	LE,
	AL,
	NV
};


struct ARM64_CPU {
	//定义Arm 64 CPU寄存器结构体
	uint64_t X[32] = { 0 };

	uint64_t negative = 0;
	uint64_t zero = 0;
	uint64_t carry = 0;
	uint64_t overflow = 0;
	Memory mem;

	template <typename T>
	struct ALUResult {
		T value;
		uint64_t nzcv[4];
	};

	inline bool is_w(Reg r) {
		return r < 32;
	}

	inline bool is_x(Reg r) {
		return r >= 32;
	}

	inline void show_nzcv() {
		std::cout << "N (Negative) = " << this->negative << std::endl;
		std::cout << "Z (Zero)     = " << this->zero << std::endl;
		std::cout << "C (Carry)    = " << this->carry << std::endl;
		std::cout << "V (Overflow) = " << this->overflow << std::endl;
	}

	//读取reg
	const uint64_t readReg(Reg r) {
		if (is_x(r)) {
			return this->X[r & 31];
		}
		if (is_w(r)) {
			return (uint32_t)this->X[r];
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//写寄存器，mov Rn, imm
	void writeReg(Reg r, uint64_t val) {
		if (is_x(r)) {
			this->X[r & 31] = val;
			return;
		}
		if (is_w(r)) {
			this->X[r] = (uint32_t)val;
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//带进位加法
	//updata nzcv
	template<typename T>
	ALUResult<T> addWithCarry(T x, T y, uint64_t carry_in) {
		using S = typename std::make_signed<T>::type;

		uint128_t unsigned_sum = uint128_t(x) + y + carry_in;
		int128_t signed_sum = int128_t((S)x) + (S)y + carry_in;

		ALUResult<T> alu_result;
		alu_result.value = unsigned_sum.convert_to<T>();

		alu_result.nzcv[3] = alu_result.value >> (sizeof(T) * 8 - 1) & 1;
		alu_result.nzcv[2] = alu_result.value == 0 ? 1 : 0;
		alu_result.nzcv[1] = unsigned_sum != cpp_int(alu_result.value) ? 1 : 0;
		alu_result.nzcv[0] = signed_sum != cpp_int((S)alu_result.value) ? 1 : 0;

		return alu_result;
	}

	template<typename T>
	T shiftReg(T value, ShiftType shift_type, uint64_t amount) {
		using S = typename std::make_signed<T>::type;
		switch (shift_type)
		{
		case LSL:
			return value << amount;
		case LSR:
			return value >> amount;
		case ASR:
			return (S)value >> amount;
		case ROR:
			return (value >> amount) | (value << ((sizeof(T) << 3) - amount));
		default:
			throw std::invalid_argument("Invalid shift type");
		}
	}

	template<typename T, typename RetType>
	RetType extend(T val, RetType, bool isUnsigned)
	{
		using S = typename std::make_signed<T>::type;
		if (isUnsigned) {
			return val;
		}
		return (S)val;
	}

	template<typename T, typename RetType>
	RetType extendReg(T value, Extend exttype, uint64_t shift, RetType realtype)
	{
		int len;

		bool isUnsigned;

		switch (exttype)
		{
		case UXTB:
			len = 8;
			isUnsigned = true;
			break;

		case UXTH:
			len = 16;
			isUnsigned = true;
			break;

		case UXTW:
			len = 32;
			isUnsigned = true;
			break;

		case UXTX:
			len = 64;
			isUnsigned = true;
			break;

		case SXTB:
			len = 8;
			isUnsigned = false;
			break;

		case SXTH:
			len = 16;
			isUnsigned = false;
			break;

		case SXTW:
			len = 32;
			isUnsigned = false;
			break;

		case SXTX:
			len = 64;
			isUnsigned = false;
			break;
		}
		int N = sizeof(T) * 8;
		int nbits = len < N ? len : N;
		RetType extval;
		switch (nbits)
		{
		case 8:
			extval = extend((uint8_t)value, realtype, isUnsigned);
			break;
		case 16:
			extval = extend((uint16_t)value, realtype, isUnsigned);
			break;
		case 32:
			extval = extend((uint32_t)value, realtype, isUnsigned);
			break;
		case 64:
			extval = extend((uint64_t)value, realtype, isUnsigned);
			break;
		}
		return extval << shift;
	}

	bool conditionHolds(Cond cond) {

		bool result;

		switch ((static_cast<int>(cond) >> 1) & 0b111)
		{
		case 0b000:
			result = (this->zero == 1);                         // EQ / NE
			break;

		case 0b001:
			result = (this->carry == 1);                        // CS / CC
			break;

		case 0b010:
			result = (this->negative == 1);                     // MI / PL
			break;

		case 0b011:
			result = (this->overflow == 1);                     // VS / VC
			break;

		case 0b100:
			result = (this->carry == 1 && zero == 0);           // HI / LS
			break;

		case 0b101:
			result = (this->negative == overflow);              // GE / LT
			break;

		case 0b110:
			result = (this->negative == overflow && zero == 0); // GT / LE
			break;

		case 0b111:
			result = true;                                // AL
			break;
		}

		// cond<0> == 1 && cond != 1111
		// invert
		if ((cond & 1) && cond != NV)
		{
			result = !result;
		}

		return result;
	}

	inline void update_nzcv(const uint64_t* nzcv) {
		this->negative = nzcv[3];
		this->zero = nzcv[2];
		this->carry = nzcv[1];
		this->overflow = nzcv[0];
	}

	//add Rd, Rn, Rm
	//add Rd, Rn, #imm
	template <typename T>
	void add(Reg rd, Reg rn, T val) {
		if (this->is_x(rd) && this->is_x(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], this->X[val & 31], (uint64_t)0);
				X[rd & 31] = alu_result.value;
				return;
			}
			else {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], (uint64_t)val, (uint64_t)0);
				X[rd & 31] = alu_result.value;
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)this->X[val], (uint64_t)0);
				X[rd] = alu_result.value;
				return;
			}
			else {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)val, (uint64_t)0);
				X[rd] = alu_result.value;
				return;
			}
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//add Rd, Rn, Rm, Extend,amount
	void add(Reg rd, Reg rn, Reg rm, Extend extended, uint64_t amount = 0) {
		if (this->is_x(rd) && this->is_x(rn)) {
			ALUResult<uint64_t> alu_result;
			if (this->is_x(rm)) {
				alu_result = addWithCarry(this->X[rn & 31], extendReg(this->X[rm & 31], extended, amount, this->X[rn & 31]), 0);
				this->X[rd & 31] = alu_result.value;
				return;
			}

			if (this->is_w(rm)) {
				alu_result = addWithCarry(this->X[rn & 31], extendReg((uint32_t)this->X[rm], extended, amount, this->X[rn & 31]), 0);
				this->X[rd & 31] = alu_result.value;
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], extendReg((uint32_t)this->X[rm], extended, amount, (uint32_t)this->X[rn]), 0);
			X[rd] = alu_result.value;
			return;
		}

		throw std::invalid_argument("Invalid Reg");

	}

	//add Rd, Rn, Rm
	//add Rd, Rn, #imm
	//updata nzcv
	template <typename T>
	void adds(Reg rd, Reg rn, T val) {
		if (this->is_x(rd) && this->is_x(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], this->X[val & 31], (uint64_t)0);
				if (rd != X31) {
					this->X[rd & 31] = alu_result.value;
				}
				update_nzcv(alu_result.nzcv);
				return;
			}
			else {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], (uint64_t)val, (uint64_t)0);
				if (rd != X31) {
					this->X[rd & 31] = alu_result.value;
				}
				update_nzcv(alu_result.nzcv);
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)this->X[val], (uint64_t)0);

				if (rd != W31) {
					X[rd] = alu_result.value;
				}
				update_nzcv(alu_result.nzcv);
				return;
			}
			else {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)val, (uint64_t)0);
				if (rd != W31) {
					X[rd] = alu_result.value;
				}
				update_nzcv(alu_result.nzcv);
				return;
			}
		}

		throw std::invalid_argument("Invalid Reg");
	}


	//add Rd, Rn, Rm, Extend,amount
	//updata nzcv
	void adds(Reg rd, Reg rn, Reg rm, Extend extended, uint64_t amount = 0) {
		if (this->is_x(rd) && this->is_x(rn)) {
			ALUResult<uint64_t> alu_result;
			if (this->is_x(rm)) {
				alu_result = addWithCarry(this->X[rn & 31], extendReg(this->X[rm & 31], extended, amount, this->X[rn & 31]), 0);
				this->X[rd & 31] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}

			if (this->is_w(rm)) {
				alu_result = addWithCarry(this->X[rn & 31], extendReg((uint32_t)this->X[rm], extended, amount, this->X[rn & 31]), 0);
				this->X[rd & 31] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], extendReg((uint32_t)this->X[rm], extended, amount, (uint32_t)this->X[rn]), 0);
			X[rd] = alu_result.value;
			update_nzcv(alu_result.nzcv);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//adc Rd, Rn, Rm
	//adc Rd, Rn, #imm
	template <typename T>
	void adc(Reg rd, Reg rn, T val) {
		if (this->is_x(rd) && this->is_x(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], this->X[val & 31], carry);
				X[rd & 31] = alu_result.value;
				return;
			}
			else {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], (uint64_t)val, carry);
				X[rd & 31] = alu_result.value;
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)this->X[val], carry);
				X[rd] = alu_result.value;
				return;
			}
			else {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)val, carry);
				X[rd] = alu_result.value;
				return;
			}
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//adcs Rd, Rn, Rm
	//adcs Rd, Rn, #imm
	//updata nzcv
	template <typename T>
	void adcs(Reg rd, Reg rn, T val) {
		if (this->is_x(rd) && this->is_x(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], this->X[val & 31], carry);
				X[rd & 31] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
			else {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], (uint64_t)val, carry);
				X[rd & 31] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)this->X[val], carry);
				X[rd] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
			else {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], (uint32_t)val, carry);
				X[rd] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
		}

		throw std::invalid_argument("Invalid Reg");
	}

	//sub Rd, Rn, Rm
	//sub Rd, Rn, #imm
	template <typename T>
	void sub(Reg rd, Reg rn, T val) {
		if (this->is_x(rd) && this->is_x(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], ~this->X[val & 31], (uint64_t)1);
				X[rd & 31] = alu_result.value;
				return;
			}
			else {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], ~(uint64_t)val, (uint64_t)1);
				X[rd & 31] = alu_result.value;
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], ~(uint32_t)this->X[val], (uint64_t)1);
				X[rd] = alu_result.value;
				return;
			}
			else {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], ~(uint32_t)val, (uint64_t)1);
				X[rd] = alu_result.value;
				return;
			}
		}
	}

	//sub Rd, Rn, Rm
	//sub Rd, Rn, #imm
	//update nzcv
	template <typename T>
	void subs(Reg rd, Reg rn, T val) {
		if (this->is_x(rd) && this->is_x(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], ~this->X[val & 31], (uint64_t)1);
				X[rd & 31] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
			else {
				ALUResult<uint64_t> alu_result = addWithCarry(this->X[rn & 31], ~(uint64_t)val, (uint64_t)1);
				X[rd & 31] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			if constexpr (std::is_same_v<T, Reg>) {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], ~(uint32_t)this->X[val], (uint64_t)1);
				X[rd] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
			else {
				ALUResult<uint32_t> alu_result = addWithCarry((uint32_t)this->X[rn], ~(uint32_t)val, (uint64_t)1);
				X[rd] = alu_result.value;
				update_nzcv(alu_result.nzcv);
				return;
			}
		}
	}


	//default LDR <Rn>, [<Xn|SP>], #<simm>
	//postIndex = false, needScale = false;  LDR <Rn>, [<Xn | SP>, #<simm>]!
	//postIndex = false, needScale = true;	 LDR <Rn>, [<Xn | SP>{, #<pimm>}]
	void ldr(Reg rd, Reg rn, int64_t val, bool postIndex = true, bool needScale = false) {
		if (!postIndex) {
			if (!needScale) {
				this->X[rn & 31] += val;
			}
			else {
				if (this->is_x(rd)) {
					uint64_t offset = this->X[rn & 31] + (val << 3);
					this->X[rd & 31] = this->mem.read64(offset);
					return;
				}
				else {
					uint64_t offset = this->X[rn & 31] + (val << 2);
					this->X[rd & 31] = this->mem.read32(offset);
					return;
				}
			}
		}
		if (this->is_x(rd)) {
			this->X[rd & 31] = this->mem.read64(this->X[rn & 31]);
			if (postIndex) {
				this->X[rn & 31] += val;
			}
			return;
		}
		if (this->is_w(rd)) {
			this->X[rd] = this->mem.read32(this->X[rn & 31]);
			if (postIndex) {
				this->X[rn & 31] += val;
			}
			return;
		}
	}


	//default LDR <Wt>, [<Xn|SP>], #<simm>
	//postIndex = false, needScale = false;  LDR <Wt>, [<Xn | SP>, #<simm>]!
	//postIndex = false, needScale = true;	 LDR <Wt>, [<Xn | SP>{, #<pimm>}]
	void ldrb(Reg rd, Reg rn, int64_t val, bool postIndex = true, bool needScale = false) {
		if (!postIndex) {
			if (!needScale) {
				this->X[rn & 31] += val;
			}
			else {
				uint64_t offset = this->X[rn & 31] + val;
				this->X[rd] = this->mem.read8(offset);
				return;

			}
		}
		this->X[rd] = this->mem.read8(this->X[rn & 31]);
		if (postIndex) {
			this->X[rn & 31] += val;
		}
		return;
	}

	//LDRB <Wt>, [<Xn | SP>, (<Wm> | <Xm>), <extend> {<amount>}]
	void ldrb(Reg rd, Reg rn, Reg rm, Extend extended, uint64_t amount = 0) {

		if (this->is_x(rm)) {
			this->X[rd] = this->mem.read8(this->X[rn & 31] + extendReg(this->X[rm & 31], extended, amount, this->X[rn & 31]));
			return;
		}

		if (this->is_w(rm)) {
			this->X[rd] = this->mem.read8(this->X[rn & 31] + extendReg((uint32_t)this->X[rm], extended, amount, this->X[rn & 31]));
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//LDURB <Wt>, [<Xn | SP>{, #<simm>}]
	void ldurb(Reg rd, Reg rn, int64_t val) {
		this->X[rd] = this->mem.read8(this->X[rn & 31] + val);
		return;
	}

	//default str <Rn>, [<Xn|SP>], #<simm>
	//postIndex = false, needScale = false;  str <Rn>, [<Xn | SP>, #<simm>]!
	//postIndex = false, needScale = true;	 str <Rn>, [<Xn | SP>{, #<pimm>}]
	void str(Reg rd, Reg rn, int64_t val, bool postIndex = true, bool needScale = false) {
		if (!postIndex) {
			if (!needScale) {
				this->X[rn & 31] += val;
			}
			else {
				if (this->is_x(rd)) {
					uint64_t offset = this->X[rn & 31] + (val << 3);
					this->mem.write64(offset, this->X[rd & 31]);
					return;
				}
				if (this->is_w(rd)) {
					uint64_t offset = this->X[rn & 31] + (val << 2);
					this->mem.write32(offset, (uint32_t)this->X[rd]);
					return;
				}
			}
		}
		if (this->is_x(rd)) {
			this->mem.write64(this->X[rn & 31], this->X[rd & 31]);
			if (postIndex) {
				this->X[rn & 31] += val;
			}
			return;
		}
		if (this->is_w(rd)) {
			this->mem.write32(this->X[rn & 31], (uint32_t)this->X[rd]);
			if (postIndex) {
				this->X[rn & 31] += val;
			}
			throw std::invalid_argument("Invalid Reg");
		}
	}

	//default strb <Wt>, [<Xn|SP>], #<simm>
	//postIndex = false, needScale = false;  strb <Wt>, [<Xn | SP>, #<simm>]!
	//postIndex = false, needScale = true;	 strb <Wt>, [<Xn | SP>{, #<pimm>}]
	void strb(Reg rd, Reg rn, int64_t val, bool postIndex = true, bool needScale = false) {
		if (!postIndex) {
			if (!needScale) {
				this->X[rn & 31] += val;
			}
			else {
				uint64_t offset = this->X[rn & 31] + val;
				this->mem.write8(offset, (uint8_t)this->X[rd]);
				return;
			}
		}
		this->mem.write8(this->X[rn & 31], (uint8_t)this->X[rd]);
		if (postIndex) {
			this->X[rn & 31] += val;
		}
		return;
	}

	//ORR <Rd>, <Rn>, <Rm>{, <shift> #<amount>}
	void orr(Reg rd, Reg rn, Reg rm, ShiftType shift_type, uint64_t amount) {
		if (this->is_x(rd) && this->is_x(rn) && this->is_x(rm)) {

			this->X[rd & 31] = this->X[rn & 31] | shiftReg(this->X[rm & 31], shift_type, amount);
			return;
		}
		if (this->is_w(rd) && this->is_w(rn) && this->is_w(rm)) {
			this->X[rd] = (uint32_t)this->X[rn] | shiftReg((uint32_t)this->X[rm], shift_type, amount);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//ORN <Rd>, <Rn>, <Rm>{, <shift> #<amount>}
	void orn(Reg rd, Reg rn, Reg rm, ShiftType shift_type, uint64_t amount) {
		if (this->is_x(rd) && this->is_x(rn) && this->is_x(rm)) {
			this->X[rd & 31] = this->X[rn & 31] | ~shiftReg(this->X[rm & 31], shift_type, amount);
			return;
		}
		if (this->is_w(rd) && this->is_w(rn) && this->is_w(rm)) {
			this->X[rd] = (uint32_t)this->X[rn] | ~shiftReg((uint32_t)this->X[rm], shift_type, amount);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//MVN <Rd>, <Rm>{, <shift> #<amount>}
	void mvn(Reg rd, Reg rm, ShiftType shift_type = LSL, uint64_t amount = 0) {
		if (this->is_x(rd) && this->is_x(rm)) {
			orn(rd, X31, rm, shift_type, amount);
			return;
		}
		if (this->is_w(rd) && this->is_w(rm)) {
			orn(rd, W31, rm, shift_type, amount);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//ANDS <Rd>, <Rn>, <Rm>{, <shift> #<amount>}
	void ands(Reg rd, Reg rn, Reg rm, ShiftType shift_type = LSL, uint64_t amount = 0) {
		uint64_t nzcv[4] = { 0 };
		if (this->is_x(rd) && this->is_x(rn) && this->is_x(rm)) {
			uint64_t result = this->X[rn & 31] & shiftReg(this->X[rm & 31], shift_type, amount);
			if (rd != X31) {
				this->X[rd] = result;
			}
			nzcv[3] = result >> 63 & 1;
			nzcv[2] = result == 0 ? 1 : 0;
			update_nzcv(nzcv);
			return;
		}
		if (this->is_w(rd) && this->is_w(rn) && this->is_w(rm)) {
			uint32_t result = (uint32_t)this->X[rn] & shiftReg((uint32_t)this->X[rm], shift_type, amount);
			if (rd != W31) {
				this->X[rd] = result;
			}
			nzcv[3] = result >> 31 & 1;
			nzcv[2] = result == 0 ? 1 : 0;
			update_nzcv(nzcv);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//MVN <Rd>, <Rm>{, <shift> #<amount>}
	void ands(Reg rd, Reg rn, uint64_t val) {
		uint64_t nzcv[4] = { 0 };
		if (this->is_x(rd) && this->is_x(rn)) {
			uint64_t result = this->X[rn & 31] & val;
			if (rd != X31) {
				this->X[rd] = result;
			}
			nzcv[3] = result >> 63 & 1;
			nzcv[2] = result == 0 ? 1 : 0;
			update_nzcv(nzcv);
			return;
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			uint32_t result = (uint32_t)this->X[rn] & (uint32_t)val;
			if (rd != W31) {
				this->X[rd] = result;
			}
			nzcv[3] = result >> 31 & 1;
			nzcv[2] = result == 0 ? 1 : 0;
			update_nzcv(nzcv);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//TST <Rn>, <Rm>{, <shift> #<amount>}
	void tst(Reg rn, Reg rm, ShiftType shift_type = LSL, uint64_t amount = 0) {
		if (this->is_x(rn) && this->is_x(rm)) {
			ands(X31, rn, rm, shift_type, amount);
			return;
		}
		if (this->is_w(rn) && this->is_w(rm)) {
			ands(W31, rn, rm, shift_type, amount);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//TST <Rn>, #<imm>
	void tst(Reg rn, uint64_t val) {
		if (this->is_x(rn)) {
			ands(X31, rn, val);
			return;
		}
		if (this->is_w(rn)) {
			ands(W31, rn, val);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//ASR <Rd>, <Rn>, #<shift>
	void asr(Reg rd, Reg rn, uint64_t amount) {
		if (this->is_x(rd) && this->is_x(rn)) {
			this->X[rd & 31] = shiftReg(this->X[rn & 31], ASR, amount);
			return;
		}
		if (this->is_w(rd) && this->is_w(rn)) {
			this->X[rd] = shiftReg((uint32_t)this->X[rn], ASR, amount);
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//CSINC <Rd>, <Rn>, <Rm>, <cond>
	void csinc(Reg rd, Reg rn, Reg rm, Cond cond) {
		if (this->is_x(rd) && this->is_x(rn) && this->is_x(rm)) {
			if (conditionHolds(cond)) {
				this->X[rd & 31] = this->X[rn & 31];
			}
			else {
				this->X[rd & 31] = this->X[rm & 31] + 1;
			}
			return;
		}
		if (this->is_w(rd) && this->is_w(rn) && this->is_w(rm)) {
			if (conditionHolds(cond)) {
				this->X[rd] = (uint32_t)this->X[rn];
			}
			else {
				this->X[rd] = (uint32_t)this->X[rm] + 1;
			}
			return;
		}
		throw std::invalid_argument("Invalid Reg");
	}

	//CSINC <Wd>, <Wn>, <Wm>, <cond>
	void cinc(Reg rd, Reg rn, Cond cond) {
		csinc(rd, rn, rn, cond);
	}



	//条件判断
	inline bool b_cs() {
		return carry == 1;
	}
	inline bool b_cc() {
		return carry == 0;
	}
	inline bool cbz(Reg rd) {
		if (is_x(rd)) {
			return this->X[rd & 31] == 0;

		}
		if (is_w(rd)) {
			return (uint32_t)this->X[rd] == 0;

		}
	}
	inline bool b_ne()
	{
		return zero == 0;
	}
};
