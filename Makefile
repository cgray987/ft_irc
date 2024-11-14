# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: fvonsovs <fvonsovs@student.42prague.com    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/06/10 14:55:00 by cgray             #+#    #+#              #
<<<<<<< HEAD
#    Updated: 2024/11/14 16:58:05 by cgray            ###   ########.fr        #
=======
#    Updated: 2024/11/14 16:57:42 by fvonsovs         ###   ########.fr        #
>>>>>>> 6867c722a775bf43b20fdd124082098f54cc1b49
#                                                                              #
# **************************************************************************** #

NAME = ircserv

CC = c++
<<<<<<< HEAD
CFLAGS = -g -std=c++98 #-Wall -Werror -Wextra
=======
CFLAGS = -std=c++98 #-Wall -Werror -Wextra -g 
>>>>>>> 6867c722a775bf43b20fdd124082098f54cc1b49

SRC = main.cpp Server.cpp User.cpp ServerCommands.cpp Channel.cpp
OBJ = ${SRC:.cpp = .o}

#Colors:
GREEN		=	\e[92;5;118m
GRAY		=	\e[33;2;37m
ITALIC		=	\e[33;3m
RED			:=	\033[31m
BCYAN		:=	\033[96m
NC			:=	\033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(ITALIC) $(BCYAN)[ 🖩 Compiling $(NAME)... 🖩 ]$(NC)"
	@ $(CC) $(CFLAGS) $(OBJ) -o $(NAME)
	@echo "$(ITALIC) $(GREEN)[ ❕$(NAME) ready❕ ]$(NC)"

clean:
	@rm -rf $(NAME)
	@echo "$(RED) [ $(NAME) Removed. ]"

fclean:
	@rm -rf $(NAME)
	@echo "$(RED) [ $(NAME) Removed. ]"

re: clean all
