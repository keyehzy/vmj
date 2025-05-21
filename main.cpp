// VM program

// 1:
//   Store $5
//   LoadImmediate 0
//   SetLocal 0
//   Load $5
//   LoadImmediate undefined
//   Store $6
//   Jump @4
// 2:
// 3:
//    LoadImmediate undefined
//    Jump @5
// 4:
//   GetLocal 0
//   Store $7
//   LoadImmediate 1000000
//   LessThan $7
//   JumpConditional true:@3 false:@6
// 5:
//   Store $6
//   GetLocal 0
//   Increment
//   SetLocal 0
//   Jump @4
// 6:
//   Load $6
//   Jump @2

#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

typedef uint64_t u64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef u64 VM_Value;
typedef u64 VM_Register;
typedef u64 VM_Local;

template <typename T, typename U>
constexpr T narrow_cast(U &&u) noexcept {
  return static_cast<T>(std::forward<U>(u));
}

struct Instruction {
  enum class Type {
    Exit,
    LoadImmediate,
    Load,
    Store,
    SetLocal,
    GetLocal,
    Increment,
    Jump,
    JumpConditional,
    LessThan,
  };

  Type type{};

  virtual ~Instruction() = default;

  virtual void dump() const = 0;

 protected:
  explicit Instruction(Type type) : type(type) {}
};

struct BasicBlock {
  std::vector<std::unique_ptr<Instruction>> instructions;
  size_t offset{};
  std::vector<size_t> jumps_to_here;

  template <typename T, typename... Args>
  void append(Args &&...args) {
    instructions.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
  }
};

struct Program {
  std::vector<std::unique_ptr<BasicBlock>> blocks;

  BasicBlock &make_block() {
    blocks.emplace_back(std::make_unique<BasicBlock>());
    return *blocks.back();
  }

  void dump() const {
    for (const auto &block : blocks) {
      std::printf("%p:\n", block.get());
      for (const auto &instruction : block->instructions) {
        std::printf("  ");
        instruction->dump();
      }
    }
  }
};

struct Exit : public Instruction {
  Exit() : Instruction(Type::Exit) {}

  void dump() const override { std::printf("Exit\n"); }
};

struct LoadImmediate : public Instruction {
  VM_Value value;

  LoadImmediate(VM_Value value) : Instruction(Type::LoadImmediate), value(value) {}

  void dump() const override { std::printf("LoadImmediate $%lu\n", value); }
};

struct Store : public Instruction {
  VM_Register reg{0};

  Store(VM_Register reg) : Instruction(Type::Store), reg(reg) {}

  void dump() const override { std::printf("Store Reg(%lu)\n", reg); }
};

struct Load : public Instruction {
  VM_Register reg{0};

  Load(VM_Register reg) : Instruction(Type::Load), reg(reg) {}

  void dump() const { std::printf("Load Reg(%lu)\n", reg); }
};

struct SetLocal : public Instruction {
  VM_Local local{0};

  SetLocal(VM_Local local) : Instruction(Type::SetLocal), local(local) {}

  void dump() const override { std::printf("SetLocal %lu\n", local); }
};

struct GetLocal : public Instruction {
  VM_Local local{0};

  GetLocal(VM_Local local) : Instruction(Type::GetLocal), local(local) {}

  void dump() const override { std::printf("GetLocal %lu\n", local); }
};

struct Increment : public Instruction {
  Increment() : Instruction(Type::Increment) {}

  void dump() const override { std::printf("Increment\n"); }
};

struct Jump : public Instruction {
  BasicBlock &target_block;

  Jump(BasicBlock &target_block) : Instruction(Type::Jump), target_block(target_block) {}

  void dump() const override { std::printf("Jump %p\n", &target_block); }
};

struct JumpConditional : public Instruction {
  BasicBlock &true_block;
  BasicBlock &false_block;

  JumpConditional(BasicBlock &true_block, BasicBlock &false_block)
      : Instruction(Type::JumpConditional), true_block(true_block), false_block(false_block) {}

  void dump() const override {
    std::printf("JumpConditional (%p) : (%p)\n", &true_block, &false_block);
  }
};

struct LessThan : public Instruction {
  VM_Register lhs{0};

  LessThan(VM_Register lhs) : Instruction(Type::LessThan), lhs(lhs) {}

  void dump() const override { std::printf("LessThan Reg(%lu)\n", lhs); }
};

struct Executable {
  Executable(size_t size) : size(size) {
    data =
        mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (data == MAP_FAILED) {
      throw std::runtime_error("Memory allocation failed");
    }
  }

  ~Executable() {
    if (data != MAP_FAILED) {
      munmap(data, size);
    }
  }

  void finalize() {
    if (mprotect(data, size, PROT_READ | PROT_EXEC) != 0) {
      throw std::runtime_error("Failed to change memory protection");
    }
  }

  void *data;
  size_t size;
};

struct Assembler {
  Assembler(std::vector<u8> &buf) : buf(buf) {}
  std::vector<u8> &buf;

  enum class Reg {
    // General purpose registers
    R0 = 0,  // RAX
    R1 = 1,  // RCX

    // VM registers
    RegisterArrayBase = 6,  // RSI
    LocalArrayBase    = 2,  // RDX
  };

  struct Operand {
    enum class Type {
      Reg,
      Imm64,
      Mem64BaseAndOffset,
    };

    Type type;
    Reg reg;
    u64 offset_or_immediate;

    static Operand Register(Reg reg) {
      Operand result{};
      result.type = Type::Reg;
      result.reg  = reg;
      return result;
    }

    static Operand Imm64(u64 immediate) {
      Operand result{};
      result.type                = Type::Imm64;
      result.offset_or_immediate = immediate;
      return result;
    }

    static Operand Mem64BaseAndOffset(Reg base, u64 offset) {
      Operand result{};
      result.type                = Type::Mem64BaseAndOffset;
      result.reg                 = base;
      result.offset_or_immediate = offset;
      return result;
    }
  };

  void mov(Operand dst, Operand src) {
    if (dst.type == Operand::Type::Reg && src.type == Operand::Type::Reg) {
      // MOV reg, reg
      emit8(0x48);
      emit8(0x89);
      emit8(0xc0 | (narrow_cast<u8>(dst.reg) << 3) | narrow_cast<u8>(src.reg));
      return;
    }

    if (dst.type == Operand::Type::Reg && src.type == Operand::Type::Imm64) {
      // MOV reg, imm64
      emit8(0x48);
      emit8(0xb8 | narrow_cast<u8>(dst.reg));
      emit64(src.offset_or_immediate);
      return;
    }

    if (dst.type == Operand::Type::Mem64BaseAndOffset && src.type == Operand::Type::Reg) {
      // MOV qword [base + offset], reg
      emit8(0x48);
      emit8(0x89);
      emit8(0x80 | (narrow_cast<u8>(src.reg) << 3) | (narrow_cast<u8>(dst.reg)));
      emit32(dst.offset_or_immediate);
      return;
    }

    if (dst.type == Operand::Type::Reg && src.type == Operand::Type::Mem64BaseAndOffset) {
      // MOV reg, qword [base + offset]
      emit8(0x48);
      emit8(0x8b);
      emit8(0x80 | (narrow_cast<u8>(dst.reg) << 3) | (narrow_cast<u8>(src.reg)));
      emit32(src.offset_or_immediate);
      return;
    }

    throw std::runtime_error("Unsupported MOV operation");
  }

  void emit8(u8 byte) { buf.push_back(byte); }

  void emit16(u16 word) {
    buf.push_back(word & 0xff);
    buf.push_back((word >> 8) & 0xff);
  }

  void emit32(u32 dword) {
    buf.push_back(dword & 0xff);
    buf.push_back((dword >> 8) & 0xff);
    buf.push_back((dword >> 16) & 0xff);
    buf.push_back((dword >> 24) & 0xff);
  }

  void emit64(u64 qword) {
    buf.push_back(qword & 0xff);
    buf.push_back((qword >> 8) & 0xff);
    buf.push_back((qword >> 16) & 0xff);
    buf.push_back((qword >> 24) & 0xff);
    buf.push_back((qword >> 32) & 0xff);
    buf.push_back((qword >> 40) & 0xff);
    buf.push_back((qword >> 48) & 0xff);
    buf.push_back((qword >> 56) & 0xff);
  }

  void load_immediate64(Reg dst, u64 value) { mov(Operand::Register(dst), Operand::Imm64(value)); }

  void store_vm_register(VM_Register dst, Reg src) {
    mov(Operand::Mem64BaseAndOffset(Reg::RegisterArrayBase, dst * sizeof(VM_Register)),
        Operand::Register(src));
  }

  void load_vm_register(Reg dst, VM_Register src) {
    mov(Operand::Register(dst),
        Operand::Mem64BaseAndOffset(Reg::RegisterArrayBase, src * sizeof(VM_Register)));
  }

  void store_vm_local(VM_Local local, Reg src) {
    mov(Operand::Mem64BaseAndOffset(Reg::LocalArrayBase, local * sizeof(VM_Local)),
        Operand::Register(src));
  }

  void load_vm_local(Reg dst, VM_Local local) {
    mov(Operand::Register(dst),
        Operand::Mem64BaseAndOffset(Reg::LocalArrayBase, local * sizeof(VM_Local)));
  }

  void increment(Reg reg) {
    emit8(0x48);
    emit8(0xff);
    emit8(0xc0 | narrow_cast<u8>(reg));
  }

  void less_than(Reg dst, Reg src) {
    // CMP src, dst
    emit8(0x48);
    emit8(0x39);
    emit8(0xc0 | (narrow_cast<u8>(src) << 3) | narrow_cast<u8>(dst));

    // SETL dst
    emit8(0x0f);
    emit8(0x9c);
    emit8(0xc0 | narrow_cast<u8>(dst));

    // MOVZX dst, dst
    emit8(0x48);
    emit8(0x0f);
    emit8(0xb6);
    emit8(0xc0 | narrow_cast<u8>(dst) << 3 | narrow_cast<u8>(dst));
  }

  void jump(BasicBlock &target_block) {
    // jmp target_block (RIP-relative 32-bit offset)
    emit8(0xe9);
    target_block.jumps_to_here.push_back(narrow_cast<u32>(buf.size()));
    emit32(0xdeadbeef);  // placeholder, will patch later
  }

  void jump_conditional(Reg cond, BasicBlock &true_block, BasicBlock &false_block) {
    // if reg != 0, jump to false_block, else jump to true_block
    emit8(0x48);
    emit8(0x83);
    emit8(0xf8);
    emit8(0x00 | narrow_cast<u8>(cond));

    // jz false_target (RIP-related 32-bit offset)
    emit8(0x0f);
    emit8(0x84);
    false_block.jumps_to_here.push_back(narrow_cast<u32>(buf.size()));
    emit32(0xdeadbeef);  // placeholder, will patch later

    // jmp true_target (RIP-related 32-bit offset)
    jump(true_block);
  }

  void exit() { emit8(0xc3); }
};

struct Jit {
  void compile_load_immediate(LoadImmediate const &instruction) {
    assembler.load_immediate64(Assembler::Reg::R0, instruction.value);
    assembler.store_vm_register(VM_Register(0), Assembler::Reg::R0);
  }

  void compile_load(Load const &instruction) {
    assembler.load_vm_register(Assembler::Reg::R0, instruction.reg);
    assembler.store_vm_register(VM_Register(0), Assembler::Reg::R0);
  }

  void compile_store(Store const &instruction) {
    assembler.load_vm_register(Assembler::Reg::R0, VM_Register(0));
    assembler.store_vm_register(instruction.reg, Assembler::Reg::R0);
  }

  void compile_get_local(GetLocal const &instruction) {
    assembler.load_vm_local(Assembler::Reg::R0, instruction.local);
    assembler.store_vm_register(VM_Register(0), Assembler::Reg::R0);
  }

  void compile_set_local(SetLocal const &instruction) {
    assembler.load_vm_register(Assembler::Reg::R0, VM_Register(0));
    assembler.store_vm_local(instruction.local, Assembler::Reg::R0);
  }

  void compile_increment(Increment const &instruction) {
    assembler.load_vm_register(Assembler::Reg::R0, VM_Register(0));
    assembler.increment(Assembler::Reg::R0);
    assembler.store_vm_register(VM_Register(0), Assembler::Reg::R0);
  }

  void compile_less_than(LessThan const &instruction) {
    assembler.load_vm_register(Assembler::Reg::R0, instruction.lhs);
    assembler.load_vm_register(Assembler::Reg::R1, VM_Register(0));
    assembler.less_than(Assembler::Reg::R0, Assembler::Reg::R1);
    assembler.store_vm_register(VM_Register(0), Assembler::Reg::R0);
  }

  void compile_jump(Jump const &instruction) { assembler.jump(instruction.target_block); }

  void compile_jump_conditional(JumpConditional const &instruction) {
    assembler.load_vm_register(Assembler::Reg::R0, VM_Register(0));
    assembler.jump_conditional(Assembler::Reg::R0, instruction.true_block, instruction.false_block);
  }

  void compile_exit(Exit const &instruction) { assembler.exit(); }

  static Executable compile(const Program &program) {
    Executable executable(4096);

    Jit jit;

    for (auto &block : program.blocks) {
      block->offset = jit.buf.size();
      for (auto &instruction : block->instructions) {
        switch (instruction->type) {
          case Instruction::Type::LoadImmediate:
            jit.compile_load_immediate(*static_cast<LoadImmediate *>(instruction.get()));
            break;
          case Instruction::Type::Load:
            jit.compile_load(*static_cast<Load *>(instruction.get()));
            break;
          case Instruction::Type::Store:
            jit.compile_store(*static_cast<Store *>(instruction.get()));
            break;
          case Instruction::Type::SetLocal:
            jit.compile_set_local(*static_cast<SetLocal *>(instruction.get()));
            break;
          case Instruction::Type::GetLocal:
            jit.compile_get_local(*static_cast<GetLocal *>(instruction.get()));
            break;
          case Instruction::Type::Increment:
            jit.compile_increment(*static_cast<Increment *>(instruction.get()));
            break;
          case Instruction::Type::LessThan:
            jit.compile_less_than(*static_cast<LessThan *>(instruction.get()));
            break;
          case Instruction::Type::Jump:
            jit.compile_jump(*static_cast<Jump *>(instruction.get()));
            break;
          case Instruction::Type::JumpConditional:
            jit.compile_jump_conditional(*static_cast<JumpConditional *>(instruction.get()));
            break;
          case Instruction::Type::Exit:
            jit.compile_exit(*static_cast<Exit *>(instruction.get()));
            break;
          default:
            throw std::runtime_error("Unknown instruction type");
        }
      }
    }

    for (auto &block : program.blocks) {
      for (auto &jump : block->jumps_to_here) {
        auto offset = block->offset - jump - 4;

        jit.buf[jump + 0] = (offset >> 0) & 0xff;
        jit.buf[jump + 1] = (offset >> 8) & 0xff;
        jit.buf[jump + 2] = (offset >> 16) & 0xff;
        jit.buf[jump + 3] = (offset >> 24) & 0xff;
      }
    }

    std::copy(jit.buf.begin(), jit.buf.end(), (u8 *)executable.data);
    executable.finalize();
    return executable;
  }

  std::vector<u8> buf;
  Assembler assembler{buf};
};

struct VM {
  std::vector<VM_Register> registers;
  std::vector<VM_Value> locals;

  void dump() const {
    std::printf("Registers:\n");
    for (size_t i = 0; i < registers.size(); ++i) {
      std::printf("  %lu: %lu\n", i, registers[i]);
    }
    std::printf("Locals:\n");
    for (size_t i = 0; i < locals.size(); ++i) {
      std::printf("  %lu: %lu\n", i, locals[i]);
    }
  }

  void interpret(const Program &program) {
    auto *current_block      = program.blocks[0].get();
    size_t instruction_index = 0;
    for (;;) {
      if (instruction_index >= current_block->instructions.size()) {
        break;
      }
      auto &instruction = current_block->instructions[instruction_index];
      switch (instruction->type) {
        case Instruction::Type::LoadImmediate:
          registers[0] = static_cast<LoadImmediate *>(instruction.get())->value;
          break;
        case Instruction::Type::Load:
          registers[0] = registers[static_cast<Load *>(instruction.get())->reg];
          break;
        case Instruction::Type::Store:
          registers[static_cast<Store *>(instruction.get())->reg] = registers[0];
          break;
        case Instruction::Type::SetLocal:
          locals[static_cast<SetLocal *>(instruction.get())->local] = registers[0];
          break;
        case Instruction::Type::GetLocal:
          registers[0] = locals[static_cast<GetLocal *>(instruction.get())->local];
          break;
        case Instruction::Type::Increment:
          registers[0]++;
          break;
        case Instruction::Type::LessThan:
          registers[0] = registers[static_cast<LessThan *>(instruction.get())->lhs] < registers[0];
          break;
        case Instruction::Type::Jump:
          current_block     = &static_cast<Jump *>(instruction.get())->target_block;
          instruction_index = 0;
          continue;
        case Instruction::Type::JumpConditional:
          if (registers[0]) {
            current_block = &static_cast<JumpConditional *>(instruction.get())->true_block;
          } else {
            current_block = &static_cast<JumpConditional *>(instruction.get())->false_block;
          }
          instruction_index = 0;
          continue;
        case Instruction::Type::Exit:
          break;
        default:
          throw std::runtime_error("Unknown instruction type");
      }
      instruction_index++;
    }
  }

  void jit(const Program &program) {
    auto executable = Jit::compile(program);

    // write(STDOUT_FILENO, executable.data, executable.size);

    // RDI: VM&
    // RSI: VM_Register* registers
    // RDX: VM_Local* locals
    typedef void (*JitFunction)(VM &, VM_Register *registers, VM_Local *locals);
    auto func = reinterpret_cast<JitFunction>(executable.data);
    func(*this, registers.data(), locals.data());
  }
};

int main() {
  auto program = Program();
  auto &block1 = program.make_block();
  auto &block2 = program.make_block();
  auto &block3 = program.make_block();
  auto &block4 = program.make_block();
  auto &block5 = program.make_block();
  auto &block6 = program.make_block();

  block1.append<Store>(VM_Register(5));
  block1.append<LoadImmediate>(VM_Value(0));
  block1.append<SetLocal>(VM_Local(0));
  block1.append<Load>(VM_Register(5));
  block1.append<LoadImmediate>(VM_Value(0));
  block1.append<Store>(VM_Register(6));
  block1.append<Jump>(block4);

  block2.append<Exit>();

  block3.append<LoadImmediate>(VM_Value(0));
  block3.append<Jump>(block5);

  block4.append<GetLocal>(VM_Local(0));
  block4.append<Store>(VM_Register(7));
  block4.append<LoadImmediate>(VM_Value(10000000));
  block4.append<LessThan>(VM_Register(7));
  block4.append<JumpConditional>(block3, block6);

  block5.append<Store>(VM_Register(6));
  block5.append<GetLocal>(VM_Local(0));
  block5.append<Increment>();
  block5.append<SetLocal>(VM_Local(0));
  block5.append<Jump>(block4);

  block6.append<Load>(VM_Register(6));
  block6.append<Jump>(block2);

  program.dump();

  auto vm = VM();
  vm.registers.resize(8);
  vm.locals.resize(8);
  vm.jit(program);
  //   vm.interpret(program);
  vm.dump();

  return 0;
}