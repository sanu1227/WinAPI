// Run from the repository root in an x64 Native Tools Command Prompt:
// cl /nologo /EHsc /utf-8 /DUNICODE /D_UNICODE tests\BasicWindowInputTests.cpp /Fe:BasicWindowInputTests.exe user32.lib gdi32.lib && BasicWindowInputTests.exe
#include <cassert>
#include <cstdio>
#include "../Weeks04-05/Project2/Project2/BasicWindow.cpp"

int main()
{
    TCHAR text[MaxLines][MaxColumns + 1]{};
    int lengths[MaxLines]{};
    int line = 0;
    int column = 0;
    auto input = [&](WPARAM character) {
        HandleCharacter(character, text, lengths, line, column);
        assert(line >= 0 && line < MaxLines);
        assert(column >= 0 && column <= MaxColumns);
        assert(column == lengths[line]);
        for (int row = 0; row < MaxLines; ++row) {
            assert(lengths[row] >= 0 && lengths[row] <= MaxColumns);
            assert(text[row][lengths[row]] == _T('\0'));
            assert(_tcslen(text[row]) == static_cast<size_t>(lengths[row]));
        }
    };

    input(VK_BACK); // Deleting an empty buffer must not underflow.
    input(VK_TAB);
    input(VK_ESCAPE);
    input(L'가'); // Preserve the exercise's ASCII-only input policy.
    assert(line == 0 && column == 0 && text[0][0] == _T('\0'));

    for (int i = 0; i < MaxColumns; ++i) input('A');
    assert(column == MaxColumns && lengths[0] == MaxColumns);
    input('X'); // The 31st character must leave the full line unchanged.
    assert(column == MaxColumns && text[0][MaxColumns - 1] == _T('A'));
    input(VK_BACK);
    input('Z');
    assert(column == MaxColumns && text[0][MaxColumns - 1] == _T('Z'));

    input(VK_RETURN);
    input('B');
    input('C');
    assert(line == 1 && column == 2 && _tcscmp(text[1], _T("BC")) == 0);
    input(VK_BACK);
    assert(_tcscmp(text[1], _T("B")) == 0);
    input(VK_BACK);
    input(VK_BACK); // Removing the line break preserves the previous line.
    assert(line == 0 && column == MaxColumns && lengths[1] == 0);

    for (int row = 1; row < MaxLines; ++row) {
        input(VK_RETURN);
        for (int i = 0; i < MaxColumns; ++i) input('A' + row);
    }
    input(VK_RETURN); // No wraparound or overwrite at the last line.
    input('X');
    assert(line == MaxLines - 1 && column == MaxColumns);
    assert(text[0][MaxColumns - 1] == _T('Z'));
    assert(text[MaxLines - 1][MaxColumns - 1] == _T('J'));

    // Enter on an existing row selects its end without overwriting its text.
    line = 0;
    column = lengths[0];
    input(VK_RETURN);
    assert(line == 1 && column == MaxColumns && text[1][MaxColumns - 1] == _T('B'));
    line = MaxLines - 1;
    column = lengths[line];

    for (int i = 0; i < MaxLines * (MaxColumns + 1) + 5; ++i) input(VK_BACK);
    assert(line == 0 && column == 0);
    for (int row = 0; row < MaxLines; ++row) assert(lengths[row] == 0);
    input(VK_RETURN);
    input(VK_RETURN);
    input(' ');
    input('~');
    assert(line == 2 && _tcscmp(text[2], _T(" ~")) == 0);
    std::puts("BasicWindow input regression checks passed.");
}
