NAME = ircserv

DEBUG ?= 0

CC = c++
CXXFLAGS = -std=c++98 -g

ifeq ($(DEBUG),1)
    CPPFLAGS += -DDEBUG
endif

SRC = main.cpp Server.cpp User.cpp ServerCommands.cpp Channel.cpp Log.cpp
OBJ = $(SRC:.cpp=.o)

# Colors:
GREEN		=	\e[92;5;118m
GRAY		=	\e[33;2;37m
ITALIC		=	\e[33;3m
RED			=	\033[31m
BCYAN		=	\033[96m
NC			=	\033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(ITALIC) $(BCYAN)[ 🖩 Linking $(NAME)... 🖩 ]$(NC)"
	@$(CC) $(OBJ) -o $(NAME)
	@echo "$(ITALIC) $(GREEN)[ ❕$(NAME) ready❕ ]$(NC)"

%.o: %.cpp
	@echo "$(ITALIC) $(GRAY)[ Compiling $< ]$(NC)"
	@$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ)
	@echo "$(RED) [ Object files removed. ]$(NC)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED) [ $(NAME) removed. ]$(NC)"

re: fclean all

debug:
	$(MAKE) DEBUG=1 re
