#pragma once

#include <iostream>

std::ostream& err(const char* module = nullptr);
std::ostream& info(const char* module = nullptr);
std::ostream& pp_stream();

[[ noreturn ]] void exit_fail();
