import sys
from sys import argv
from enum import Enum


class state(Enum):
    EMPTY = 0
    WHITE = 1
    BLACK = -1


class status(Enum):
    WRONG_MOVE = 0
    NORMAL_MOVE = 1
    EATING_MOVE = 2


class game_result(Enum):
    TIE = 0
    FIRST = 1
    SECOND = 2
    ILLEGAL = 3


class left_or_right(Enum):
    RIGHT = 1
    LEFT = -1


# global variables
board = None
must_eat = False
whos_turn = state.WHITE  # TODO we are assuming the white is always first
balance = 0


# global variables

def get_initialized_board(rows=8, columns=8):
    global board
    board = [[state.EMPTY for j in range(columns)] for i in range(rows)]
    for i in range(rows):
        for j in range(columns):
            if (i + j) % 2:
                if i < 3:
                    board[i][j] = state.WHITE
                elif (4 < i) and (i < 8):
                    board[i][j] = state.BLACK
    return board


def understand_move(move):
    if (not is_the_target_space_empty(move)) or (not is_the_move_starts_with_the_right_piece(move)):
        return status.WRONG_MOVE

    if is_there_any_possible_eating_move():
        if is_the_move_is_valid_eating_move(move):
            return status.EATING_MOVE
        else:
            return status.WRONG_MOVE
    else:
        if is_the_move_jumping_is_valid(move, status.NORMAL_MOVE):
            return status.NORMAL_MOVE


    return status.WRONG_MOVE


def is_the_move_is_valid_eating_move(move):
    global board
    start_column, start_row, next_column, next_row = move[0], move[1], move[2], move[3]
    ##TODO make it global only in the understand_move scope

    if (not is_in_boundaries(start_row,start_row)):
        return False
    if (not is_in_boundaries(next_row, next_column)):
        return False

    if (not (next_row-start_row == 2*whos_turn.value)):
        return False

    if (next_column-start_column == 2):
        return is_eating_possible_direction_diagonal(start_row, start_column, left_or_right.RIGHT)
    if (next_column-start_column == -2):
        return is_eating_possible_direction_diagonal(start_row, start_column, left_or_right.LEFT)

    return False


def is_the_move_jumping_is_valid(move, mode):  # TODO change the func name
    start_column, start_row, next_column, next_row = move[0], move[1], move[2], move[3]
    return (mode.value == abs(next_column - start_column)) and \
           ((next_row - start_row) == mode.value * whos_turn.value)


#
# def is_the_move_is_valid_normal_move(move):
#     start_column, start_row, next_column, next_row = move[0], move[1], move[2], move[3]
#     return (1 == abs(next_column - start_column)) and ((next_row - start_row) == whos_turn.value)
#

def is_the_target_space_empty(move):
    global board
    next_column, next_row = move[2], move[3]
    result = state.EMPTY == board[next_row][next_column]
    return result


def is_the_move_starts_with_the_right_piece(move):
    global board
    start_column, start_row = move[0], move[1]
    return (whos_turn == board[start_row][start_column] )#TODO change back


# def run_rules_and_classified(move):
#     global is_in_eating_mode
#     the_move = understand_move(move)
#     if status.WRONG_MOVE == the_move:
#         return status.WRONG_MOVE
#     if status.NORMAL_MOVE == the_move:
#         return the_move
#     if status.EATING_MOVE == the_move:
#         is_in_eating_mode = True
#         return the_move


def perform_move(move, move_result):
    assert (move_result != status.WRONG_MOVE)
    if status.NORMAL_MOVE == move_result:
        perform_normal_move(move)
    if status.EATING_MOVE == move_result:
        perform_eating_move(move)


def perform_normal_move(move):
    global board
    start_column, start_row, next_column, next_row = move[0], move[1], move[2], move[3]
    board[next_row][next_column] = board[start_row][start_column]
    board[start_row][start_column] = state.EMPTY
    change_whos_turn()


def perform_eating_move(move):
    do_the_eating_move(move)
    update_balance()
    if (not is_there_any_possible_eating_move_for_piece(move[3], move[2])):
        change_whos_turn()


def do_the_eating_move(move):
    global board
    start_column, start_row, next_column, next_row = move[0], move[1], move[2], move[3]
    board[(next_row + start_row) // 2][(next_column + start_column) // 2] = state.EMPTY
    board[next_row][next_column] = board[start_row][start_column]
    board[start_row][start_column] = state.EMPTY


def update_balance():
    global balance
    balance += whos_turn.value


def who_won_according_to_the_balance():
    global balance
    if 0 == balance:
        return game_result.TIE
    if balance > 0:
        return game_result.FIRST
    else:
        return game_result.SECOND


def change_whos_turn():
    global whos_turn
    assert whos_turn != state.EMPTY
    whos_turn = the_appose_turn()


def the_appose_turn():
    global whos_turn
    assert whos_turn != state.EMPTY
    if whos_turn == state.WHITE:
        return state.BLACK
    else:
        return state.WHITE


def get_illegal_move_string(move, line_number):
    return "line : " + "%d" % line_number + " illegal move: " + "%s" % str(move)


# def get_game_result_str(move_result, move, line_number):
#     if (FIRST == move_result):
#         return "first"
#     if (SECOND == move_result):
#         return "second"
#     if (TIE == move_result):
#         return "tie"
#     if (ILLEGAL == move_result):
#         return get_illigal_move_string(move, line_number)
#
#     assert (False)
#     return "unknown error"


def is_there_any_possible_move():
    global board
    global whos_turn #TODO for making it readable - alwyas declare what are the globals we're using

    for i in range(len(board)):
        for j in range(len(board[i])):
            if (whos_turn.value == board[i][j].value) and (is_there_any_possible_move_for_piece(i, j)):
                return True
    return False


def is_there_any_possible_move_for_piece(row, column):
    return is_there_any_possible_eating_move_for_piece(row, column) or \
           is_there_any_possible_simple_move_for_piece(row, column)


def is_there_any_possible_eating_move():
    global board
    for i in range(len(board)):
        for j in range(len(board)):
            if (whos_turn.value == board[i][j].value) and (is_there_any_possible_eating_move_for_piece(i, j)):
                print ("possible eating move from %d %d",i,j)
                return True
    return False


def is_there_any_possible_eating_move_for_piece(start_row, start_column):
    global board

    if is_this_piece_is_at_the_2_last_rows(start_row):  # TODO length, not 7
        return False  # the piece is not able to move forward from this point the edge of the board   #TODO spelling

    if (is_eating_possible_direction_diagonal(start_row, start_column, left_or_right.RIGHT) or
            is_eating_possible_direction_diagonal(start_row, start_column, left_or_right.LEFT)):
        return True
    return False


def is_this_piece_is_at_the_2_last_rows(start_row):
    expected_row = start_row + 2 * whos_turn.value
    return len(board) < expected_row or 0 > expected_row


def is_in_boundaries(row, column):
    if ((row >= 0 and row < len(board)) and (column >= 0 and column < len(board))):
        return True
    return False


def is_oponent_exists(row, column):
    assert (is_in_boundaries(row, column))
    if (board[row][column] == the_appose_turn()):
        return True
    return False


def is_free_space_after_oponent(row, column):
    assert (is_in_boundaries(row, column))
    if (board[row][column] == state.EMPTY):
        return True
    return False


def is_eating_possible_direction_diagonal(start_row, start_column, left_or_right):
    expected_column = start_column + 2 * left_or_right.value
    expected_row = start_row + 2 * whos_turn.value
    if (not is_in_boundaries(expected_row, expected_column)):
        return False

    if (is_oponent_exists(start_row + 1 * whos_turn.value, start_column + 1 * left_or_right.value) and
        (is_free_space_after_oponent(start_row + 2 * whos_turn.value, start_column + 2 * left_or_right.value))):
        return True

    return False
    #
    # if (the_appose_turn() == board[start_row + whos_turn.value][start_column + direction.value]) and \
    #         (state.EMPTY == board[start_row + 2 * whos_turn.value][start_column + 2 * direction.value]):
    #     return True
    # return False


def is_there_any_possible_simple_move_for_piece(row, column):
    global board
    if (0<= (row + whos_turn.value) and 8> (row + whos_turn.value)):
        if (column < 7 and board[row + whos_turn.value][column + 1] == state.EMPTY) or \
                (column > 0 and board[row + whos_turn.value][column - 1] == state.EMPTY):
            print("possible simple move from %d %d", row, column)
            return True #TODO not 0,7,8... #TODO divide to several files
    return False


def run_game(moves):
    global board
    board = get_initialized_board()
    # is_in_eating_mode = False
    # whos_turn = WHITE  # TODO we are assuming the white is always first
    # balance = 0

    for i in range(len(moves)):
        print ("\n Turn number is ", i)
        print_board(moves[i])
        move_result = understand_move(moves[i])
        if status.WRONG_MOVE != move_result:
            perform_move(moves[i], move_result)
        else:
            return get_illegal_move_string(moves[i], i + 1)
    print_board(moves[0])
    if is_there_any_possible_move():
        return "incomplete"
    return str(who_won_according_to_the_balance())


def read_input(file_name):
    f_input = open(file_name, 'r')
    new_move = f_input.readline()
    moves = []
    while (new_move is not None) and len(new_move) > 0:
        lst = [int(e) for e in new_move.split(",")]
        if is_a_not_valid_line(lst):
            return None
        moves.append(lst)
        new_move = f_input.readline()
    return moves


def is_a_not_valid_line(lst):
    if len(lst) != 4:
        return True
    for e in lst:
        if e < 0 or e > 7:
            return True
    return False


def main():
    moves = read_input(argv[1])
    if moves is None:
        print("Error: Bad input file")
        return -1

    result_str = run_game(moves)
    print(result_str)
    return 0

def print_board(move):

    print ("BOARD")
    print ("Move is ", move)
    print ("Turn is ", whos_turn)
    for i in range (len(board)):
        print (i, end="")
        for j in range (len(board)):
            res = " . "
            if((board[i][j].value) == 1):
                res = " X "
            if ((board[i][j].value) == -1):
                res = " V "

            print(res,end="")
        print("")

if __name__ == "__main__":  # See why to use this kind of mains stackoverflow.com/questions/4041238/why-use-def-main
    main()
