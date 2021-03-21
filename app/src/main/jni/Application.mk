APP_CFLAGS   := -Wall -Wextra -Wshadow -Werror -fvisibility=hidden
APP_CFLAGS   += -D_LIBCPP_DISABLE_EXTERN_TEMPLATE -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION
APP_CPPFLAGS := -std=c++20
APP_STL      := system
