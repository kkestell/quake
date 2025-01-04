import argparse
import curses
import time
from typing import List, Tuple


def parse_args():
    parser = argparse.ArgumentParser(description='Interactive search and replace utility using ncurses.')
    parser.add_argument('directory', type=str, help='The directory to search in.')
    parser.add_argument('search', type=str, help='The string to search for.')
    parser.add_argument('replacement', type=str, help='The string to replace the search string with.')
    return parser.parse_args()


# def display_content(stdscr, content, highlight_row, highlight_col_start, highlight_col_end, highlight_color):
#     num_rows, num_cols = stdscr.getmaxyx()

#     stdscr.move(0, 0)
#     stdscr.clear()

#     if len(content) < num_rows:
#         for row_idx, row in enumerate(content):
#             line = row[:num_cols].rstrip()
#             if row_idx == highlight_row - 1:
#                 stdscr.addstr(line[:highlight_col_start])
#                 stdscr.addstr(line[highlight_col_start:highlight_col_end], highlight_color)
#                 stdscr.addstr(line[highlight_col_end:].rstrip())
#             else:
#                 stdscr.addstr(line)
#             if row_idx < len(content) - 1:
#                 stdscr.addstr('\n')
#     else:
#         if highlight_row <= num_rows and highlight_col_end <= num_cols:
#             for row_idx, row in enumerate(content[:num_rows]):
#                 line = row[:num_cols].rstrip()
#                 if row_idx == highlight_row - 1:
#                     stdscr.addstr(line[:highlight_col_start])
#                     stdscr.addstr(line[highlight_col_start:highlight_col_end], highlight_color)
#                     stdscr.addstr(line[highlight_col_end:].rstrip())
#                 else:
#                     stdscr.addstr(line)
#                 if row_idx < num_rows - 1:
#                     stdscr.addstr('\n')
#         else:
#             # Determine the vertical scroll position
#             if highlight_row > num_rows:
#                 start_row = max(0, highlight_row - num_rows // 2)
#             else:
#                 start_row = 0

#             # Determine the horizontal scroll position
#             if highlight_col_end > num_cols:
#                 start_col = max(0, highlight_col_start - num_cols // 2)
#             else:
#                 start_col = 0

#             # Display the content with scrolling
#             for row_idx, row in enumerate(content[start_row:start_row + num_rows]):
#                 line = row[start_col:start_col + num_cols].rstrip()
#                 if row_idx + start_row == highlight_row - 1:
#                     local_start = max(0, highlight_col_start - start_col)
#                     local_end = min(num_cols, highlight_col_end - start_col)
#                     stdscr.addstr(line[:local_start])
#                     stdscr.addstr(line[local_start:local_end], highlight_color)
#                     stdscr.addstr(line[local_end:].rstrip())
#                 else:
#                     stdscr.addstr(line,)
#                 if row_idx < num_rows - 1:
#                     stdscr.addstr('\n')

#     stdscr.refresh()


def find_occurrences(content, search):
    """
    Find all occurrences of a search string in a list of strings. Returns a list of tuples, where each tuple contains
    the line number and the start and end column of the occurrence.
    """
    occurrences = []
    row = 0
    col = 0

    idx = 0
    while idx < len(content):
        if content[idx:idx + 2] == '//':
            while content[idx] != '\n':
                idx += 1
                col += 1
            idx += 1
            row += 1
            col = 0
        # elif content[idx:idx + 2] == '/*':
        #     while content[idx:idx + 2] != '*/':
        #         if content[idx] == '\n':
        #             row += 1
        #             col = 0
        #         idx += 1
        #         col += 1
        #     idx += 2
        # elif content[idx] == '"':
        #     idx += 1
        #     col += 1
        #     while content[idx] != '"':
        #         idx += 1
        #         col += 1
        #     idx += 1
        #     col += 1
        elif content[idx] == '\n':
            row += 1
            col = 0
            idx += 1
        else:
            prev_char = content[idx - 1] if idx > 0 else None
            next_char = content[idx + len(search)] if idx + len(search) < len(content) else None

            if content[idx:idx + len(search)] == search:
                if prev_char is not None and (prev_char.isalpha() or prev_char.isnumeric() or prev_char == '_'):
                    col += 1
                    idx += 1
                    continue

                if next_char is not None and (next_char.isalpha() or next_char.isnumeric() or next_char == '_'):
                    col += 1
                    idx += 1
                    continue

                occurrences.append((row + 1, col, col + len(search)))
                col += len(search)
                idx += len(search)
            else:
                col += 1
                idx += 1

    return occurrences


# def init_curses(stdscr):
#     curses.noecho()
#     curses.cbreak()
#     stdscr.keypad(True)
#     curses.curs_set(0)
#     curses.start_color()
#     curses.init_color(0, 0,    0,    0)
#     curses.init_color(1, 1000, 1000, 1000)
#     curses.init_color(2, 700,  700,  700)
#     curses.init_color(3, 1000, 1000, 749)
#     curses.init_color(4, 564, 933, 564)
#     curses.init_pair(1, 1, 0)
#     curses.init_pair(2, 0, 3)
#     curses.init_pair(3, 0, 4)
#     stdscr.bkgd(' ', curses.color_pair(1))
#     stdscr.clear()


# def shutdown_curses(stdscr):
#     curses.nocbreak()
#     stdscr.keypad(False)
#     curses.echo()
#     curses.endwin()


def main(stdscr, directory, search, replacement):
    # find all .c and .h files in the directory
    import os
    files = []
    for root, _, filenames in os.walk(directory):
        for filename in filenames:
            if filename.endswith('.c') or filename.endswith('.h'):
                files.append(os.path.join(root, filename))

    # init_curses(stdscr)

    for filename in files:
        print(f'Processing {filename}...')
        with open(filename, 'r') as file:
            content = file.read()

        idx = 0 # which occurrence we are currently on
        while True:
            occurrences = find_occurrences(content, search)
            if not occurrences:
                break

            if idx >= len(occurrences):
                idx = 0

            lines = content.split('\n')
            line_num, start_col, end_col = occurrences[idx]

            # display_content(stdscr, lines, line_num, start_col, end_col, curses.color_pair(2))

            # ch = stdscr.getch()
            # if ch == 10:
            lines[line_num - 1] = lines[line_num - 1][:start_col] + replacement + lines[line_num - 1][end_col:]
            content = '\n'.join(lines)
                # display_content(stdscr, lines, line_num, start_col, start_col + len(replacement), curses.color_pair(3))
                # time.sleep(0.5)
            # elif ch == curses.KEY_UP:
                # idx -= 1
            # elif ch == curses.KEY_DOWN:
                # idx += 1
            # if ch == ord('q'):
                # break

        with open(filename, 'w') as file:
            file.write(content)

    # shutdown_curses(stdscr)


if __name__ == '__main__':
    args = parse_args()
    # curses.wrapper(main, args.directory, args.search, args.replacement)
    main(None, args.directory, args.search, args.replacement)
