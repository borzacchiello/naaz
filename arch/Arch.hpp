#pragma once

#include <cstdlib>
#include <filesystem>

namespace naaz
{

enum Endianess { LITTLE, BIG };

class Arch
{
  protected:
    static std::filesystem::path getSleighProcessorDir();

  public:
    virtual std::filesystem::path getSleighSLA() const   = 0;
    virtual std::filesystem::path getSleighPSPEC() const = 0;
    virtual Endianess             endianess() const      = 0;
};

namespace arch
{

class x86_64 : public naaz::Arch
{
  private:
    x86_64() {}

  public:
    virtual std::filesystem::path getSleighSLA() const;
    virtual std::filesystem::path getSleighPSPEC() const;
    virtual Endianess endianess() const { return Endianess::LITTLE; }

    static const x86_64& The()
    {
        static x86_64 singleton;
        return singleton;
    }
};

} // namespace arch
} // namespace naaz
