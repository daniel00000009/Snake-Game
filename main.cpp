#include <opencv2/opencv.hpp>
#include <iostream>
#include <ctime>
#include <deque>
#include <cstdlib>
#include <fstream>

using namespace cv;
using namespace std;

const int WIDTH = 600;
const int HEIGHT = 400;
const int CELL_SIZE = 20;
const string HIGH_SCORE_FILE = "highscore.txt";
int windowWidth = WIDTH;
int windowHeight = HEIGHT;

// Globals to track the window size
//int g_window_width = 0, g_window_height = 0;

int buttonPressed;

enum Direction { UP, DOWN, LEFT, RIGHT };

enum GameStates { MENU, PLAYING, OPTIONS, EXIT, GAME_OVER };

struct SnakePoint{
    int x, y;
    SnakePoint(int x = 0, int y = 0) : x(x), y(y) {}
};

class SnakeGame
{
private:
    deque<SnakePoint> snake;
    SnakePoint apple;
    SnakePoint specialApple;
    SnakePoint pinkApple;
    int numHearts;
    Direction dir;
    bool gameOver;
    size_t gameScore;
    size_t highScore;
    bool isPaused;
    bool isInvincible;
    int64_t invincibilityEndTime;
    const int INVINCIBILITY_DURATION = 10;
    const int SUPERPOWER_DURATION = 20;
    int normalSpeed;


    void placeApple() {
        srand(time(0));
        if (windowWidth > 0 && windowHeight > 0) {
            apple.x = (rand() % (windowWidth / CELL_SIZE)) * CELL_SIZE;
            apple.y = (rand() % (windowHeight / CELL_SIZE)) * CELL_SIZE;
        } else {
            apple.x = windowWidth / 2;
            apple.y = windowHeight / 2;
        }
    }

    void placeSpecialApple() {
        if (rand() % 100 >= 50) { // 50% to respawn
            specialApple.x = -1; 
            specialApple.y = -1;
            return;
        }

        int x, y;
        do {
            x = (rand() % (windowWidth / CELL_SIZE)) * CELL_SIZE;
            y = (rand() % (windowHeight / CELL_SIZE)) * CELL_SIZE;
        } while (isAppleOnSnake(x, y) || (x == apple.x && y == apple.y) || (x == pinkApple.x && y == pinkApple.y));
        specialApple.x = x;
        specialApple.y = y;
    }

    void placePinkApple() {
        if (rand() % 100 >= 20) { // 20% to respawn
            pinkApple.x = -1; 
            pinkApple.y = -1;
            return;
        }

        int x, y;
        do {
            x = (rand() % (windowWidth / CELL_SIZE)) * CELL_SIZE;
            y = (rand() % (windowHeight / CELL_SIZE)) * CELL_SIZE;
        } while (isAppleOnSnake(x, y) || (x == apple.x && y == apple.y) || (x == specialApple.x && y == specialApple.y));

        pinkApple.x = x;
        pinkApple.y = y;
    }

    bool isCollision(SnakePoint pt) {
        if (pt.x < 0 || pt.x >= windowWidth || pt.y < 0 || pt.y >= windowHeight)
            return true;
        for (auto& segment : snake) {
            if (segment.x == pt.x && segment.y == pt.y)
                return true;
        }
        return false;
    }


public:
	SnakeGame();
	~SnakeGame();
    void update(int &snakeSpeed);
    void changeDirection(int key);
    void render(Mat& frame);
    bool isGameOver() const { return gameOver; }
    void resetGame();
    void loadHighScore();
    void saveHighScore();
    void loseHeart();
    void drawHeart(Mat& frame, Point position);

    bool isGamePaused() const { return isPaused; }
    void togglePause() { isPaused = !isPaused; }
    bool isAppleOnSnake(int x, int y);
    void buyLife();
    void buySuperPower(int &snakeSpeed);
};

SnakeGame::SnakeGame() : dir(RIGHT), gameOver(false), gameScore(0), highScore(0), isPaused(false), numHearts(1), normalSpeed(100)
{
    snake.push_back(SnakePoint(windowWidth / 2, windowHeight / 2));
    srand((unsigned)time(0));
    placeApple();
    specialApple = SnakePoint(-1, -1);
    pinkApple = SnakePoint(-1, -1);
    loadHighScore();
}

SnakeGame::~SnakeGame() {}

void SnakeGame::update(int& snakeSpeed) {
    if (gameOver) return;

    if (getTickCount() > invincibilityEndTime) {
        snakeSpeed = normalSpeed; // Reset speed back to normal after superpower ends
    }

    if (isInvincible && getTickCount() > invincibilityEndTime) {
        isInvincible = false;
    }

    // The next head position (direction) based on the pressed key
    SnakePoint head = snake.front();
    switch (dir) {
        case UP: head.y -= CELL_SIZE; break;
        case DOWN: head.y += CELL_SIZE; break;
        case LEFT: head.x -= CELL_SIZE; break;
        case RIGHT: head.x += CELL_SIZE; break;
    }

    if (isInvincible) {
        if (head.x < 0) {
            head.x = windowWidth - CELL_SIZE; // Teleport to the right
        }
        else if (head.x >= windowWidth) {
            head.x = 0; // Teleport to the left
        }

        if (head.y < 0) {
            head.y = windowHeight - CELL_SIZE; // Teleport to the bottom
        }
        else if (head.y >= windowHeight) {
            head.y = 0; // Teleport to the top
        }
    }
    else if (isCollision(head)) {
        loseHeart();
        return;
    }

    snake.push_front(head);

    if (head.x == apple.x && head.y == apple.y) {
        placeApple();  
        gameScore++;

        if (specialApple.x == -1 && specialApple.y == -1) {
            placeSpecialApple();
        }
        if (pinkApple.x == -1 && pinkApple.y == -1) {
            placePinkApple();
        }
    } 
    else if (head.x == specialApple.x && head.y == specialApple.y) {
        gameScore += 2;
        placeSpecialApple();
    } 
    else if (head.x == pinkApple.x && head.y == pinkApple.y) {
        numHearts++;
        gameScore++;
        placePinkApple();
    } 
    else {
        snake.pop_back();  
    }


    if (gameScore > highScore) {
        highScore = gameScore;
        saveHighScore();
    }

}

void SnakeGame::changeDirection(int key) {
    switch (key) {
    case 'w': if (dir != DOWN) dir = UP; break;
    case 'a': if (dir != RIGHT) dir = LEFT; break;
    case 's': if (dir != UP) dir = DOWN; break;
    case 'd': if (dir != LEFT) dir = RIGHT; break;
    }
}

void SnakeGame::render(Mat &frame) {
    frame = Scalar(0, 0, 0);
    Scalar snakeColor = isInvincible ? Scalar(0, 255, 255) : Scalar(0, 255, 0);

    if (gameOver) {
        putText(frame, "Game Over", Point(windowWidth / 3, windowHeight / 2), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
        return;
    }

    // "Drawing" the snake body
    for (auto& segment : snake) {
        rectangle(frame, Rect(segment.x, segment.y, CELL_SIZE, CELL_SIZE), snakeColor, FILLED);
    }

    // "Drawing" the red apple
    rectangle(frame, Rect(apple.x, apple.y, CELL_SIZE, CELL_SIZE), Scalar(0, 0, 255), FILLED);

    // "Drawing" the golden apple
    if (specialApple.x != -1 && specialApple.y != -1) {
        rectangle(frame, Rect(specialApple.x, specialApple.y, CELL_SIZE, CELL_SIZE), Scalar(0, 255, 255), FILLED);
    }

    // "Drawing" the pink apple
    if (pinkApple.x != -1 && pinkApple.y != -1) {
        rectangle(frame, Rect(pinkApple.x, pinkApple.y, CELL_SIZE, CELL_SIZE), Scalar(255, 105, 180), FILLED);
    }
    
    // Drawing hearts
    for (int i = 0; i < numHearts; i++) {
        Point heartPos(10 + i * 30, 50);
        drawHeart(frame, heartPos);
    }

    putText(frame, ("Score: " + to_string(gameScore)), Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    putText(frame, ("HighScore: " + to_string(highScore)), Point(windowWidth- 160, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255, 255), 2);

    if (isInvincible) {
        int remainingTime = static_cast<int>((invincibilityEndTime - getTickCount()) / getTickFrequency());
        if (remainingTime > 0) {
            putText(frame, "Invincible: " + to_string(remainingTime) + "s",
                Point(10, windowHeight - 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255), 2);
        }
        else {
            isInvincible = false; // Deactivate the shown time
        }
    }

}

void SnakeGame::resetGame() {
    snake.clear();
    snake.push_back(SnakePoint(windowWidth / 2, windowHeight / 2));
    gameOver = false;
    gameScore = 0;
    dir = RIGHT;
    numHearts = 6;
    isInvincible = false;

    placeApple();
}

void SnakeGame::loadHighScore() {
    ifstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        if (!(file >> highScore)) { // load the highscore value in the program
            highScore = 0;
        } else {
            cout << "Error HIGH_SCORE_FILE" << endl;
        }
    }
    
}

void SnakeGame::saveHighScore() {
    ofstream file(HIGH_SCORE_FILE);

    if (file.is_open()) {
        file << highScore; // save the highscore in the text file
        file.close();
    }
    else {
        cout << "\nNu s-a putut deschide fisierulul pentru SALVARE!" << endl;
    }
}

void SnakeGame::loseHeart() {
    if (isInvincible) {
        return; // not losing hearts
    }

    numHearts--;
    if (numHearts <= 0) {
        gameOver = true;
    }
    else {
        isInvincible = true;
        invincibilityEndTime = getTickCount() + INVINCIBILITY_DURATION * getTickFrequency();
    }
}

void SnakeGame::drawHeart(Mat& frame, Point position) {
    vector<Point> heart;
    heart.push_back(Point(position.x, position.y));
    heart.push_back(Point(position.x + 20, position.y));
    heart.push_back(Point(position.x + 10, position.y + 20));
    polylines(frame, heart, true, Scalar(255, 105, 180), 2);
}

bool SnakeGame::isAppleOnSnake(int x, int y) {
    for (auto segment : snake) {
        if (segment.x == x && segment.y == y) {
            return true;
        }
    }
    return false;
}

void SnakeGame::buyLife() {
    if (highScore >= 5) {
        numHearts++;
        highScore -= 5;
    }
}

void SnakeGame::buySuperPower(int &snakeSpeed) {
    if (numHearts >= 5) {
        snakeSpeed = 70;
        isInvincible = true;
        invincibilityEndTime = getTickCount() + SUPERPOWER_DURATION * getTickFrequency();
        numHearts -= 5;
    }
}



void showMenu(Mat& frame, int& selectedOption);
void handleMenuInput(int key, int &selectedOption, GameStates &currentState, SnakeGame& game);

void showGameOverMenu(Mat& frame, int& selectedOption);
void handleGameOverMenuInput(int key, int& selectedOption, GameStates& currentState, SnakeGame& game);

void showOptionsMenu(Mat& frame, int& selectedOption, int snakeSpeed, bool soundEnable, int& windowWidth, int& windowHeight);
void handleOptionsMenuInput(int key, int& selectedOption, GameStates& currentState, int& snakeSpeed, bool& soundEnable, int& windowWidth, int& windowHeight, SnakeGame& game, Mat& frame);
//void checkWindowSize(const std::string& windowName);



int main() {
    SnakeGame game;
    Mat frame(windowHeight, windowWidth, CV_8UC3);
    namedWindow("Snake Game", WINDOW_NORMAL);

    GameStates currentState = MENU;
    int selectedOption = 0;
    int snakeSpeed = 100;
    bool soundEnable = true;

    int64_t gameOverTimeStamp = 0;

    while (currentState != EXIT) 
    {
        int key = waitKey(snakeSpeed);

        if (key == 32 && currentState == PLAYING) { // SPACE for pause
            game.togglePause();
        }

        if (game.isGamePaused()) {
            frame = Scalar(0, 0, 0);
            putText(frame, "Pause", Point(windowWidth / 3 + 20, windowHeight / 2), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
            putText(frame, "Press SPACE to Resume", Point(windowWidth / 3 - 110, windowHeight / 2 + 50), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 255, 255), 1.5);
            putText(frame, "Press 1 to Buy a Life (5 points)", Point(windowWidth / 3 - 110, windowHeight / 2 + 150), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 255, 255), 1.5);
            putText(frame, "Press 2 to Buy a Super Power (15 points)", Point(windowWidth / 3 - 110, windowHeight / 2 + 100), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 255, 255), 1.5);

            if (key == '1') {  // Check for key '1'
                game.buyLife();  // Call buyLife() function
            }
            if (key == '2') {  // Check for key '2'
                game.buySuperPower(snakeSpeed);  // Call buySuperPower() function
            }

            imshow("Snake Game", frame); 
            continue;
        }

        switch (currentState)
        {
        case MENU:
            showMenu(frame, selectedOption);
            handleMenuInput(key, selectedOption, currentState, game);
            break;

        case PLAYING:
            if (key != -1) game.changeDirection(key);
            game.update(snakeSpeed);
            game.render(frame);

            if (game.isGameOver()) {
                currentState = GAME_OVER;
                gameOverTimeStamp = getTickCount();
                selectedOption = 0;
            }
            break;

        case OPTIONS:
            showOptionsMenu(frame, selectedOption, snakeSpeed, soundEnable, windowWidth, windowHeight);
            handleOptionsMenuInput(key, selectedOption, currentState, snakeSpeed, soundEnable, windowWidth, windowHeight, game, frame);
            break;

        case EXIT:
            break;
        
        case GAME_OVER:
            game.render(frame);
            if ((getTickCount() - gameOverTimeStamp) / getTickFrequency() >= 3) {
                showGameOverMenu(frame, selectedOption);
                handleGameOverMenuInput(key, selectedOption, currentState, game);
            }
            break;
        
        }

        imshow("Snake Game", frame);
    }

    return 0;
}



void showMenu(Mat &frame, int &selectedOption) {
    vector<string> menuOptions = { "Start the Game", "Options", "Exit" };
    frame = Scalar(0, 0, 0);

    for (size_t i = 0; i < menuOptions.size(); i++) {
        Scalar color = (i == selectedOption) ? Scalar(0, 255, 0) : Scalar(255, 255, 255);
        putText(frame, menuOptions[i], Point(windowWidth / 3, 100 + i * 40), FONT_HERSHEY_SIMPLEX, 1, color, 2);
    }
}

void handleMenuInput(int key, int& selectedOption, GameStates& currentState, SnakeGame& game) {
    if (key == 'w' && selectedOption > 0) {
        selectedOption--;
    }
    if (key == 's' && selectedOption < 2) {
        selectedOption++;
    }

    if (key == 13) { // ASCI code for enter button
        switch (selectedOption) {
        case 0: 
            currentState = PLAYING; 
            game.resetGame();
            break;
        case 1: currentState = OPTIONS; break;
        case 2: currentState = EXIT; break;
        }
    }
}

void showGameOverMenu(Mat& frame, int& selectedOption) {
    vector<string> gameOverMenuOptions = { "Retry", "Back to Menu" };
    frame = Scalar(0, 0, 0);

    for (size_t i = 0; i < gameOverMenuOptions.size(); i++) {
        Scalar color = (i == selectedOption) ? Scalar(0, 255, 0) : Scalar(255, 255, 255);
        putText(frame, gameOverMenuOptions[i], Point(windowWidth / 3, windowHeight / 2 + i * 40), FONT_HERSHEY_SIMPLEX, 1, color, 2);
    }

}

void handleGameOverMenuInput(int key, int& selectedOption, GameStates& currentState, SnakeGame& game) {

    if (key == 'w' && selectedOption > 0) selectedOption--;
    if (key == 's' && selectedOption < 1) selectedOption ++;

    if (key == 13) { // ASCI code for enter button
        switch (selectedOption) {
        case 0:
            game.resetGame();
            currentState = PLAYING;
            break;
        case 1:
            currentState = MENU;
            break;
        }
    }

}

void showOptionsMenu(Mat& frame, int& selectedOption, int snakeSpeed, bool soundEnable, int& windowWidth, int& windowHeight){
    vector<string> optionsMenu = { 
        "1. Snake Speed:", 
        "2. Sound:", 
        "3. Window Size: 800 x 600",
        "4. Full-screen: 1400 x 760",
        "5. Back"
    };
    frame = Scalar(0, 0, 0);

    int textYPosition = 80;
    int lineSpacing = 50;

    for (size_t i = 0; i < optionsMenu.size(); i++) {
        Scalar color = (i == selectedOption) ? Scalar(0, 255, 0) : Scalar(255, 255, 255);
        putText(frame, optionsMenu[i], Point(50, textYPosition + i * lineSpacing), FONT_HERSHEY_SIMPLEX, 0.8, color, 2);

        if (i == 0) {
            int baseX = 165;  
            Scalar colorLent = Scalar(255, 255, 255);
            Scalar colorNormal = Scalar(255, 255, 255);
            Scalar colorRapid = Scalar(255, 255, 255);

            if (snakeSpeed == 200) {
                colorLent = Scalar(0, 255, 0);
            }
            else if (snakeSpeed == 100) {
                colorNormal = Scalar(0, 255, 0);
            }
            else if (snakeSpeed == 50) {
                colorRapid = Scalar(0, 255, 0);
            }

            putText(frame, "Slow", Point(baseX + 100, textYPosition), FONT_HERSHEY_SIMPLEX, 0.8, colorLent, 2);
            putText(frame, "|", Point(baseX + 160, textYPosition), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
            putText(frame, "Normal", Point(baseX + 170, textYPosition), FONT_HERSHEY_SIMPLEX, 0.8, colorNormal, 2);
            putText(frame, "|", Point(baseX + 270, textYPosition), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
            putText(frame, "Fast", Point(baseX + 280, textYPosition), FONT_HERSHEY_SIMPLEX, 0.8, colorRapid, 2);
        }

        if (i == 1) {
            int baseX = 200;
            Scalar colorOn = Scalar(255, 255, 255);
            Scalar colorOff = Scalar(255, 255, 255);

            if (soundEnable) {
                colorOn = Scalar(0, 255, 0);
            }
            else
            {
                colorOff = Scalar(0, 255, 0);
            }
            

            putText(frame, "On", Point(baseX, textYPosition + 53), FONT_HERSHEY_SIMPLEX, 0.8, colorOn, 2);
            putText(frame, "|", Point(baseX + 40, textYPosition + 53), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
            putText(frame, "Off", Point(baseX + 60, textYPosition + 53), FONT_HERSHEY_SIMPLEX, 0.8, colorOff, 2);
            
        }
    }
}

void handleOptionsMenuInput(int key, int& selectedOption, GameStates& currentState, int& snakeSpeed, bool& soundEnable, int& windowWidth, int& windowHeight, SnakeGame& game, Mat& frame) {
    if (key == 'w' && selectedOption > 0) selectedOption--;
    if (key == 's' && selectedOption < 4) selectedOption++;

    if (key == 13) {
        switch (selectedOption) {
        case 0:
            if (snakeSpeed == 100) snakeSpeed = 200;
            else if (snakeSpeed == 200) snakeSpeed = 50;
            else snakeSpeed = 100;
            break;

        case 1:
            soundEnable = !soundEnable;
            break;

        case 2:
            windowWidth = (windowWidth == WIDTH) ? 800 : WIDTH;
            windowHeight = (windowHeight == HEIGHT) ? 600 : HEIGHT;
            resizeWindow("Snake Game", windowWidth, windowHeight);
            frame = Mat(windowHeight, windowWidth, CV_8UC3);
            game.resetGame();
            break;

        case 3:
            windowWidth = 1400;
            windowHeight = 760;
            setWindowProperty("Snake Game", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
            frame = Mat(windowHeight, windowWidth, CV_8UC3);
            game.resetGame();
            break;

        case 4:
            currentState = MENU;
            selectedOption = 0;
            break;
        }
    }

}

// Callback function to handle window size
//void checkWindowSize(const std::string& windowName) {
//    cv::Rect rect = cv::getWindowImageRect(windowName); // Get window dimensions
//    if (rect.width != g_window_width || rect.height != g_window_height) {
//        g_window_height = rect.height;
//        g_window_height = g_window_height >> 2;
//        g_window_height = g_window_height << 2;
//        g_window_height = g_window_height - (g_window_height % CELL_SIZE);
//        //g_window_width = rect.width;
//        g_window_width = (int)((float)g_window_height * 1.5f);
//        g_window_width = g_window_width - (g_window_width % CELL_SIZE);
//        std::cout << "Window resized to: " << g_window_width << "x" << g_window_height << std::endl;
//    }
//}

