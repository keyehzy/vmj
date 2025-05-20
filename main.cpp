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

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

typedef uint64_t u64;
typedef uint8_t u8;

typedef u64 VM_Value;
typedef u64 VM_Register;
typedef u64 VM_Local;

struct Instruction {
  enum class Type {
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
};

struct BasicBlock {
  std::vector<std::unique_ptr<Instruction>> instructions;

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

struct LoadImmediate : public Instruction {
  VM_Value value;

  LoadImmediate(VM_Value value)
      : Instruction(Type::LoadImmediate), value(value) {}

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

  Jump(BasicBlock &target_block)
      : Instruction(Type::Jump), target_block(target_block) {}

  void dump() const override { std::printf("Jump %p\n", &target_block); }
};

struct JumpConditional : public Instruction {
  BasicBlock &true_block;
  BasicBlock &false_block;

  JumpConditional(BasicBlock &true_block, BasicBlock &false_block)
      : Instruction(Type::JumpConditional),
        true_block(true_block),
        false_block(false_block) {}

  void dump() const override {
    std::printf("JumpConditional (%p) : (%p)\n", &true_block, &false_block);
  }
};

struct LessThan : public Instruction {
  VM_Register lhs{0};

  LessThan(VM_Register lhs) : Instruction(Type::LessThan), lhs(lhs) {}

  void dump() const override { std::printf("LessThan Reg(%lu)\n", lhs); }
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

  block3.append<LoadImmediate>(VM_Value(0));
  block3.append<Jump>(block5);

  block4.append<GetLocal>(VM_Local(0));
  block4.append<Store>(VM_Register(7));
  block4.append<LoadImmediate>(VM_Value(1000000));
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

  auto *current_block      = &block1;
  size_t instruction_index = 0;

  for (;;) {
    if (instruction_index >= current_block->instructions.size()) {
      break;
    }
    auto &instruction = current_block->instructions[instruction_index];
    switch (instruction->type) {
      case Instruction::Type::LoadImmediate:
        vm.registers[0] =
            static_cast<LoadImmediate *>(instruction.get())->value;
        break;
      case Instruction::Type::Load:
        vm.registers[0] =
            vm.registers[static_cast<Load *>(instruction.get())->reg];
        break;
      case Instruction::Type::Store:
        vm.registers[static_cast<Store *>(instruction.get())->reg] =
            vm.registers[0];
        break;
      case Instruction::Type::SetLocal:
        vm.locals[static_cast<SetLocal *>(instruction.get())->local] =
            vm.registers[0];
        break;
      case Instruction::Type::GetLocal:
        vm.registers[0] =
            vm.locals[static_cast<GetLocal *>(instruction.get())->local];
        break;
      case Instruction::Type::Increment:
        vm.registers[0]++;
        break;
      case Instruction::Type::LessThan:
        vm.registers[0] =
            vm.registers[static_cast<LessThan *>(instruction.get())->lhs] < vm.registers[0];
        break;
      case Instruction::Type::Jump:
        current_block = &static_cast<Jump *>(instruction.get())->target_block;
        instruction_index = 0;
        continue;
      case Instruction::Type::JumpConditional:
        if (vm.registers[0]) {
          current_block =
              &static_cast<JumpConditional *>(instruction.get())->true_block;
        } else {
          current_block =
              &static_cast<JumpConditional *>(instruction.get())->false_block;
        }
        instruction_index = 0;
        continue;

      default:
        throw std::runtime_error("Unknown instruction type");
    }
    instruction_index++;
  }
  vm.dump();

  return 0;
}