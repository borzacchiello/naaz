#pragma once

#include <iostream>

std::ostream& err(const char* module = nullptr);
std::ostream& info(const char* module = nullptr);

void exit_fail();
