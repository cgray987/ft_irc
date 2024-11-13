# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: fvonsovs <fvonsovs@student.42prague.com    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/06/10 14:55:00 by cgray             #+#    #+#              #
#    Updated: 2024/11/13 15:47:42 by fvonsovs         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ircserv

CC = c++
CFLAGS = -g -std=c++98 #-Wall -Werror -Wextra -fsanitize=address -g 

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
