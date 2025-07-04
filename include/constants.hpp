#pragma once

// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define CYAN "\033[1;36m"
#define YELLOW "\033[1;33m"
#define CLEAR "\033[0m"

// Backlog value for listen. System maximum is 4096
#define BACKLOG 128

// Logging statuses
#define LOG_FAIL "\033[1;31mFAILURE\033[0m"
#define LOG_SUCCESS "\033[1;32mSUCCESS\033[0m"
