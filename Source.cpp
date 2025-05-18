#include <iostream>
#include <string>
#include <unistd.h> 
#include <cstdlib> 
#include <thread>
#include <termios.h>
#include <fcntl.h>
#include <map>
#include <poll.h>
#include <vector>
#include <ctime>

using namespace std;

// Game constants
const int nFieldWidth = 20;
const int nFieldHeight = 25;
const int nScreenWidth = 80;
const int nScreenHeight = 30;

// Tetromino definitions
wstring tetromino[7];

// Game components
struct GameObject {
    unsigned char* pField;
    int nCurrentPiece;
    int nCurrentRotation;
    int nCurrentX;
    int nCurrentY;
    int nSpeed;
    int nSpeedCounter;
    bool bForceDown;
    int nScore;
    int nLines;
    int nLevel;
    bool gameOver;
    
    // Constructor to initialize all members
    GameObject() {
        pField = nullptr;
        nCurrentPiece = 0;
        nCurrentRotation = 0;
        nCurrentX = nFieldWidth / 2;
        nCurrentY = 0;
        nSpeed = 5;
        nSpeedCounter = 0;
        bForceDown = false;
        nScore = 0;
        nLines = 0;
        nLevel = 1;
        gameOver = false;
    }
};

// Input component
struct InputSystem {
    bool bKey[4];
    
    // Constructor to initialize the array
    InputSystem() {
        bKey[0] = false;
        bKey[1] = false;
        bKey[2] = false;
        bKey[3] = false;
    }
    
    void ReadInput() {
        struct pollfd pfd = { 0, POLLIN, 0 };
        while (poll(&pfd, 1, 0) > 0) {
            pfd.revents = 0;
            char ch;
            if (read(0, &ch, 1) != 1) break;

            if (ch == '\x1b') {
                char seq[2];
                if (read(0, &seq[0], 1) != 1) break;
                if (read(0, &seq[1], 1) != 1) break;

                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'D': bKey[0] = true; break; // Left
                        case 'C': bKey[1] = true; break; // Right
                        case 'B': bKey[2] = true; break; // Down
                    }
                }
            }
            else if (ch == 'z' || ch == 'Z') {
                bKey[3] = true; // Rotate
            }
        }
    }
    
    void ResetKeys() {
        fill(begin(bKey), end(bKey), false);
    }
};

// Rendering component
struct RenderSystem {
    wstring screen;
    
    RenderSystem() : screen(nScreenWidth * nScreenHeight, L' ') {}
    
    void ClearScreen() {
        cout << "\x1B[2J\x1B[H";
    }
    
    void MoveCursorTopLeft() {
        cout << "\x1B[H";
    }
    
    void HideCursor() {
        cout << "\x1B[?25l";
    }
    
    void ShowCursor() {
        cout << "\x1B[?25h";
    }
    
    void ClearBuffer() {
        for (int i = 0; i < screen.size(); i++)
            screen[i] = L' ';
    }
    
    void DrawField(unsigned char* pField) {
        for (int y = 0; y < nFieldHeight; y++) {
            for (int x = 0; x < nFieldWidth; x++) {
                wchar_t block = pField[y * nFieldWidth + x];
                wstring ch = (block == 0) ? L"  " : (block == 9 ? L" #" : L"[]");
                screen.replace((y + 2) * nScreenWidth + (x + 2) * 2, 2, ch);
            }
        }
    }
    
    void DrawPiece(int nCurrentPiece, int nCurrentRotation, int nCurrentX, int nCurrentY) {
        for (int px = 0; px < 4; px++) {
            for (int py = 0; py < 4; py++) {
                if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] == L'X') {
                    screen[(nCurrentY + py + 2) * nScreenWidth + (nCurrentX + px + 2) * 2] = L'[';
                    screen[(nCurrentY + py + 2) * nScreenWidth + (nCurrentX + px + 2) * 2 + 1] = L']';
                }
            }   
        }
    }
    
    void DrawScore(int nScore, int nLines, int nLevel) {
        wstring scoreText = L"Score: " + to_wstring(nScore);
        wstring linesText = L"Lines: " + to_wstring(nLines);
        wstring levelText = L"Level: " + to_wstring(nLevel);
        
        for (int i = 0; i < scoreText.length(); i++)
            screen[3 * nScreenWidth + nFieldWidth * 2 + 10 + i] = scoreText[i];
            
        for (int i = 0; i < linesText.length(); i++)
            screen[5 * nScreenWidth + nFieldWidth * 2 + 10 + i] = linesText[i];
            
        for (int i = 0; i < levelText.length(); i++)
            screen[7 * nScreenWidth + nFieldWidth * 2 + 10 + i] = levelText[i];
    }
    
    void DrawControls() {
        wstring controls[] = {
            L"Controls:",
            L"Left Arrow - Move Left",
            L"Right Arrow - Move Right",
            L"Down Arrow - Move Down",
            L"Z - Rotate"
        };
        
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < controls[i].length(); j++) {
                screen[(10 + i) * nScreenWidth + nFieldWidth * 2 + 10 + j] = controls[i][j];
            }
        }
    }
    
    void Render() {
        for (int y = 0; y < nScreenHeight; y++) {
            for (int x = 0; x < nScreenWidth; x++) {
                wchar_t ch = screen[y * nScreenWidth + x];
                wcout << (ch == 0x2588 ? L"" : wstring(1, ch));
            }
            wcout << endl;
        }
    }
    
    int Rotate(int px, int py, int r) {
        switch (r % 4) {
            case 0: return py * 4 + px;
            case 1: return 12 + py - (px * 4);
            case 2: return 15 - (py * 4) - px;
            case 3: return 3 - py + (px * 4);
        }
        return 0;
    }
};

// Physics/Game Logic Component
struct PhysicsSystem {
    bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY, unsigned char* pField) {
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++) {
                int pi = Rotate(px, py, nRotation);
                int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

                if(nPosX + px >= 0 && nPosX + px < nFieldWidth) {
                    if (nPosY + py >= 0 && nPosY + py < nFieldHeight) {
                        if (tetromino[nTetromino][pi] == L'X' && pField[fi] != 0) {
                            return false;
                        }
                    }
                }
            }
        return true;
    }
    
    int Rotate(int px, int py, int r) {
        switch (r % 4) {
            case 0: return py * 4 + px;
            case 1: return 12 + py - (px * 4);
            case 2: return 15 - (py * 4) - px;
            case 3: return 3 - py + (px * 4);
        }
        return 0;
    }
    
    // Check for completed lines and update score
    void CheckForLines(GameObject& game) {
        for (int y = 0; y < nFieldHeight - 1; y++) {
            bool bLine = true;
            for (int x = 1; x < nFieldWidth - 1; x++) {
                bLine &= (game.pField[y * nFieldWidth + x]) != 0;
            }
            
            if (bLine) {
                // Line is complete - remove it and score points
                for (int x = 1; x < nFieldWidth - 1; x++)
                    game.pField[y * nFieldWidth + x] = 0;
                    
                // Move all blocks above the line down
                for (int py = y; py > 0; py--) {
                    for (int x = 1; x < nFieldWidth - 1; x++) {
                        game.pField[py * nFieldWidth + x] = game.pField[(py-1) * nFieldWidth + x];
                    }
                }
                
                // Update score - more points for higher levels
                game.nScore += 100 * game.nLevel;
                game.nLines++;
                
                // Level up every 10 lines
                if (game.nLines % 10 == 0) {
                    game.nLevel++;
                    // Speed up as level increases
                    game.nSpeed = max(1, 20 - game.nLevel);
                }
            }
        }
    }
};

// Terminal Management
void EnableRawMode() {
    termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Turn off canonical mode and echo
    tcsetattr(0, TCSANOW, &term);
}

void DisableRawMode() {
    termios term;
    tcgetattr(0, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);
}

// Game Engine
class GameEngine {
private:
    GameObject game;
    InputSystem input;
    RenderSystem renderer;
    PhysicsSystem physics;
    
public:
    GameEngine() {
        InitGame();
    }
    
    ~GameEngine() {
        delete[] game.pField;
        DisableRawMode();
        renderer.ShowCursor();
    }
    
    void InitGame() {
        // Initialize tetromino shapes
        tetromino[0].append(L"..X.");
        tetromino[0].append(L"..X.");
        tetromino[0].append(L"..X.");
        tetromino[0].append(L"..X.");

        tetromino[1].append(L"..X.");
        tetromino[1].append(L".XX.");
        tetromino[1].append(L".X..");
        tetromino[1].append(L"....");

        tetromino[2].append(L".X..");
        tetromino[2].append(L".XX.");
        tetromino[2].append(L"..X.");
        tetromino[2].append(L"....");

        tetromino[3].append(L"....");
        tetromino[3].append(L".XX.");
        tetromino[3].append(L".XX.");
        tetromino[3].append(L"....");

        tetromino[4].append(L"..X.");
        tetromino[4].append(L".XX.");
        tetromino[4].append(L"..X.");
        tetromino[4].append(L"....");

        tetromino[5].append(L"....");
        tetromino[5].append(L".XX.");
        tetromino[5].append(L"..X.");
        tetromino[5].append(L"..X.");

        tetromino[6].append(L"....");
        tetromino[6].append(L".XX.");
        tetromino[6].append(L".X..");
        tetromino[6].append(L".X..");
        
        // Create field
        game.pField = new unsigned char[nFieldWidth * nFieldHeight];
        for (int x = 0; x < nFieldWidth; x++)
            for (int y = 0; y < nFieldHeight; y++)
                game.pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;
                
        // Seed random number generator
        srand(time(nullptr));
        
        // Pick the first piece
        game.nCurrentPiece = rand() % 7;
        
        // Setup terminal
        EnableRawMode();
        atexit(DisableRawMode);
        
        // Setup display
        renderer.ClearScreen();
        renderer.MoveCursorTopLeft();
        renderer.HideCursor();
    }
    
    void ProcessInput() {
        input.ResetKeys();
        input.ReadInput();
        
        // Handle movement based on input
        if (input.bKey[0] && physics.DoesPieceFit(game.nCurrentPiece, game.nCurrentRotation, game.nCurrentX - 1, game.nCurrentY, game.pField)) 
            game.nCurrentX--;
        
        if (input.bKey[1] && physics.DoesPieceFit(game.nCurrentPiece, game.nCurrentRotation, game.nCurrentX + 1, game.nCurrentY, game.pField)) 
            game.nCurrentX++;
        
        if (input.bKey[2] && physics.DoesPieceFit(game.nCurrentPiece, game.nCurrentRotation, game.nCurrentX, game.nCurrentY + 1, game.pField)) {
            game.nCurrentY++;
            // Extra points for manually moving down
            game.nScore += 1;
        }
        
        if (input.bKey[3] && physics.DoesPieceFit(game.nCurrentPiece, game.nCurrentRotation + 1, game.nCurrentX, game.nCurrentY, game.pField)) 
            game.nCurrentRotation++;
    }
    
    void Update() {
        game.nSpeedCounter++;
        game.bForceDown = (game.nSpeedCounter == game.nSpeed);
        
        if (game.bForceDown) {
            if (physics.DoesPieceFit(game.nCurrentPiece, game.nCurrentRotation, game.nCurrentX, game.nCurrentY + 1, game.pField)) {
                game.nCurrentY++;
            }
            else {
                // Lock the piece in place
                for (int px = 0; px < 4; px++)
                    for (int py = 0; py < 4; py++)
                        if (tetromino[game.nCurrentPiece][physics.Rotate(px, py, game.nCurrentRotation)] == L'X')
                            game.pField[(game.nCurrentY + py) * nFieldWidth + (game.nCurrentX + px)] = game.nCurrentPiece + 1;
                
                // Check for completed lines and update score
                physics.CheckForLines(game);
                
                // Create new piece
                game.nCurrentX = nFieldWidth / 2;
                game.nCurrentY = 0;
                game.nCurrentRotation = 0;
                game.nCurrentPiece = rand() % 7;
                
                // Check if game over
                game.gameOver = !physics.DoesPieceFit(game.nCurrentPiece, game.nCurrentRotation, game.nCurrentX, game.nCurrentY, game.pField);
            }
            game.nSpeedCounter = 0;
        }
    }
    
    void Render() {
        renderer.MoveCursorTopLeft();
        renderer.ClearBuffer();
        
        // Draw field
        renderer.DrawField(game.pField);
        
        // Draw current piece
        renderer.DrawPiece(game.nCurrentPiece, game.nCurrentRotation, game.nCurrentX, game.nCurrentY);
        
        // Draw score information
        renderer.DrawScore(game.nScore, game.nLines, game.nLevel);
        
        // Draw controls
        renderer.DrawControls();
        
        // Render to terminal
        renderer.Render();
    }
    
    void GameLoop() {
        while (!game.gameOver) {
            this_thread::sleep_for(std::chrono::milliseconds(50));
            
            ProcessInput();
            Update();
            Render();
        }
        
        // Game over screen
        renderer.ClearScreen();
        wcout << L"Game Over!" << endl;
        wcout << L"Final Score: " << game.nScore << endl;
        wcout << L"Lines Cleared: " << game.nLines << endl;
        wcout << L"Level Reached: " << game.nLevel << endl;
    }
    
    bool IsGameOver() const {
        return game.gameOver;
    }
};

int main() {
    GameEngine engine;
    engine.GameLoop();
    
    return 0;
}