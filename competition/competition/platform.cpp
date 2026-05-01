// Local ConnectSix arena.
// It compiles/runs alice.cpp and james.cpp, swaps their colors by game,
// saves combined Botzone-like logs into result/summary-<rounds>*.json,
// and prints an Alice-vs-James win/loss summary to the terminal.

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

constexpr int GRID_SIZE = 15;
constexpr int PLAYER_BLACK = 0;
constexpr int PLAYER_WHITE = 1;
constexpr int PLAYER_TIE = 2;
constexpr int GRID_BLACK = 1;
constexpr int GRID_WHITE = -1;
constexpr int GRID_BLANK = 0;
constexpr int TIME_LIMIT_MS = 1000;  // 默认时间限制（alice严格按此值）

struct Move {
    int x0 = -1;
    int y0 = -1;
    int x1 = -1;
    int y1 = -1;
};

struct BotRun {
    bool keepRunning = false;
    int memoryKb = 0;
    long long timeMs = 1;
    string verdict = "OK";
    string raw;
    string debug;
    int exitCode = 0;
};

struct GameResult {
    int winner = PLAYER_TIE;
    bool abnormal = false;
    string reason;
    string blackName;
    string whiteName;
    string winnerName;
    vector<string> logEntries;
};

struct Summary {
    int aliceWins = 0;
    int jamesWins = 0;
    int ties = 0;
    int abnormalGames = 0;
    int aliceAsBlack = 0;
    int jamesAsBlack = 0;
};

int board[GRID_SIZE][GRID_SIZE];

string tabs(int n)
{
    return string(n, '\t');
}

string trim(const string& s)
{
    size_t first = 0;
    while (first < s.size() && isspace(static_cast<unsigned char>(s[first]))) {
        ++first;
    }
    size_t last = s.size();
    while (last > first && isspace(static_cast<unsigned char>(s[last - 1]))) {
        --last;
    }
    return s.substr(first, last - first);
}

string jsonEscape(const string& s)
{
    ostringstream out;
    for (unsigned char ch : s) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                const char* hex = "0123456789abcdef";
                out << "\\u00" << hex[(ch >> 4) & 0xf] << hex[ch & 0xf];
            } else {
                out << ch;
            }
            break;
        }
    }
    return out.str();
}

string quoteForCmd(const string& s)
{
#ifdef _WIN32
    string out = "\"";
    for (char ch : s) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out += ch;
        }
    }
    out += "\"";
    return out;
#else
    string out = "'";
    for (char ch : s) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += "'";
    return out;
#endif
}

string quoteForPowerShell(const string& s)
{
    string out = "'";
    for (char ch : s) {
        if (ch == '\'') {
            out += "''";
        } else {
            out += ch;
        }
    }
    out += "'";
    return out;
}

bool inMap(int x, int y)
{
    return x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE;
}

int colorOfPlayer(int player)
{
    return player == PLAYER_BLACK ? GRID_BLACK : GRID_WHITE;
}

void resetBoard()
{
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            board[i][j] = GRID_BLANK;
        }
    }
}

int countEmptyCells()
{
    int empty = 0;
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            if (board[i][j] == GRID_BLANK) {
                ++empty;
            }
        }
    }
    return empty;
}

bool isLegalMove(const Move& mv, int player, bool firstMove)
{
    if (!inMap(mv.x0, mv.y0)) {
        return false;
    }
    if (board[mv.x0][mv.y0] != GRID_BLANK) {
        return false;
    }

    if (firstMove) {
        return player == PLAYER_BLACK && mv.x1 == -1 && mv.y1 == -1;
    }

    if (!inMap(mv.x1, mv.y1)) {
        return false;
    }
    if (mv.x0 == mv.x1 && mv.y0 == mv.y1) {
        return false;
    }
    if (board[mv.x1][mv.y1] != GRID_BLANK) {
        return false;
    }
    return true;
}

void applyMove(const Move& mv, int player, bool firstMove)
{
    const int color = colorOfPlayer(player);
    board[mv.x0][mv.y0] = color;
    if (!firstMove) {
        board[mv.x1][mv.y1] = color;
    }
}

bool winsFromPoint(int x, int y, int color)
{
    static const int dx[4] = { 1, 0, 1, 1 };
    static const int dy[4] = { 0, 1, 1, -1 };
    for (int dir = 0; dir < 4; ++dir) {
        int cnt = 1;
        int nx = x + dx[dir];
        int ny = y + dy[dir];
        while (inMap(nx, ny) && board[nx][ny] == color) {
            ++cnt;
            nx += dx[dir];
            ny += dy[dir];
        }
        nx = x - dx[dir];
        ny = y - dy[dir];
        while (inMap(nx, ny) && board[nx][ny] == color) {
            ++cnt;
            nx -= dx[dir];
            ny -= dy[dir];
        }
        if (cnt >= 6) {
            return true;
        }
    }
    return false;
}

bool moveMakesWin(const Move& mv, int player, bool firstMove)
{
    const int color = colorOfPlayer(player);
    if (winsFromPoint(mv.x0, mv.y0, color)) {
        return true;
    }
    return !firstMove && winsFromPoint(mv.x1, mv.y1, color);
}

bool namedInt(const string& raw, const string& key, int& value)
{
    const regex re("\"?" + key + "\"?\\s*[:=]\\s*(-?\\d+)");
    smatch m;
    if (!regex_search(raw, m, re)) {
        return false;
    }
    value = stoi(m[1].str());
    return true;
}

bool parseMove(const string& raw, Move& mv)
{
    string text = trim(raw);
    if (text.empty()) {
        return false;
    }

    Move named;
    if (namedInt(text, "x0", named.x0) &&
        namedInt(text, "y0", named.y0) &&
        namedInt(text, "x1", named.x1) &&
        namedInt(text, "y1", named.y1)) {
        mv = named;
        return true;
    }

    vector<int> nums;
    const regex numRe("-?\\d+");
    for (sregex_iterator it(text.begin(), text.end(), numRe), end; it != end; ++it) {
        nums.push_back(stoi(it->str()));
        if (nums.size() == 4) {
            break;
        }
    }
    if (nums.size() < 4) {
        return false;
    }

    mv.x0 = nums[0];
    mv.y0 = nums[1];
    mv.x1 = nums[2];
    mv.y1 = nums[3];
    return true;
}

string moveObject(const Move& mv, int indent)
{
    ostringstream out;
    out << "{\n";
    out << tabs(indent + 1) << "\"x0\": " << mv.x0 << ",\n";
    out << tabs(indent + 1) << "\"y0\": " << mv.y0 << ",\n";
    out << tabs(indent + 1) << "\"x1\": " << mv.x1 << ",\n";
    out << tabs(indent + 1) << "\"y1\": " << mv.y1 << "\n";
    out << tabs(indent) << "}";
    return out.str();
}

string moveObjectCompact(const Move& mv)
{
    ostringstream out;
    out << "{\"x0\":" << mv.x0
        << ",\"y0\":" << mv.y0
        << ",\"x1\":" << mv.x1
        << ",\"y1\":" << mv.y1 << "}";
    return out.str();
}

string moveArrayCompact(const vector<Move>& moves)
{
    ostringstream out;
    out << "[";
    for (size_t i = 0; i < moves.size(); ++i) {
        if (i) {
            out << ",";
        }
        out << moveObjectCompact(moves[i]);
    }
    out << "]";
    return out.str();
}

string makeBotInput(const vector<string>& logEntries, const vector<Move>& requests,
                    const vector<Move>& responses, int player)
{
    ostringstream out;
    out << "{";
    out << "\"log\":";
    out << "[";
    for (size_t i = 0; i < logEntries.size(); ++i) {
        if (i) {
            out << ",";
        }
        out << logEntries[i];
    }
    out << "],";
    out << "\"requests\":" << moveArrayCompact(requests) << ",";
    out << "\"responses\":" << moveArrayCompact(responses) << ",";
    out << "\"color\":" << player << ",";
    out << "\"board_size\":" << GRID_SIZE;
    out << "}\n";
    return out.str();
}

string matchInfoEntry(int gameIndex, int totalRounds, const string& blackName, const string& whiteName)
{
    ostringstream out;
    out << "{\n";
    out << "\t\"type\": \"match_info\",\n";
    out << "\t\"game\": " << gameIndex << ",\n";
    out << "\t\"total_games\": " << totalRounds << ",\n";
    out << "\t\"players\": {\n";
    out << "\t\t\"0\": \"" << jsonEscape(blackName) << "\",\n";
    out << "\t\t\"1\": \"" << jsonEscape(whiteName) << "\",\n";
    out << "\t\t\"blackone\": \"" << jsonEscape(blackName) << "\",\n";
    out << "\t\t\"whiteone\": \"" << jsonEscape(whiteName) << "\",\n";
    out << "\t\t\"black\": \"" << jsonEscape(blackName) << "\",\n";
    out << "\t\t\"white\": \"" << jsonEscape(whiteName) << "\"\n";
    out << "\t}\n";
    out << "}";
    return out.str();
}

string judgeRequestEntry(int player, const Move& req, bool hasDisplay)
{
    ostringstream out;
    out << "{\n";
    out << "\t\"keep_running\": false,\n";
    out << "\t\"memory\": 0,\n";
    out << "\t\"output\": {\n";
    out << "\t\t\"command\": \"request\",\n";
    out << "\t\t\"content\": {\n";
    out << "\t\t\t\"" << player << "\": " << moveObject(req, 3) << "\n";
    out << "\t\t}";
    if (hasDisplay) {
        out << ",\n";
        out << "\t\t\"display\": " << moveObject(req, 2) << "\n";
    } else {
        out << "\n";
    }
    out << "\t},\n";
    out << "\t\"time\": 1,\n";
    out << "\t\"verdict\": \"OK\"\n";
    out << "}";
    return out.str();
}

void scoresForWinner(int winner, int& scoreBlack, int& scoreWhite)
{
    if (winner == PLAYER_BLACK) {
        scoreBlack = 2;
        scoreWhite = 0;
    } else if (winner == PLAYER_WHITE) {
        scoreBlack = 0;
        scoreWhite = 2;
    } else {
        scoreBlack = 1;
        scoreWhite = 1;
    }
}

string judgeFinishEntry(int winner, const string& err, const Move* lastMove)
{
    int scoreBlack = 1;
    int scoreWhite = 1;
    scoresForWinner(winner, scoreBlack, scoreWhite);

    ostringstream out;
    out << "{\n";
    out << "\t\"keep_running\": false,\n";
    out << "\t\"memory\": 0,\n";
    out << "\t\"output\": {\n";
    out << "\t\t\"command\": \"finish\",\n";
    out << "\t\t\"content\": {\n";
    out << "\t\t\t\"0\": " << scoreBlack << ",\n";
    out << "\t\t\t\"1\": " << scoreWhite << "\n";
    out << "\t\t},\n";
    out << "\t\t\"display\": {\n";
    if (!err.empty()) {
        out << "\t\t\t\"err\": \"" << jsonEscape(err) << "\",\n";
        out << "\t\t\t\"winner\": " << winner << "\n";
    } else {
        if (lastMove) {
            out << "\t\t\t\"x0\": " << lastMove->x0 << ",\n";
            out << "\t\t\t\"y0\": " << lastMove->y0 << ",\n";
            out << "\t\t\t\"x1\": " << lastMove->x1 << ",\n";
            out << "\t\t\t\"y1\": " << lastMove->y1 << ",\n";
        }
        out << "\t\t\t\"winner\": " << winner << "\n";
    }
    out << "\t\t}\n";
    out << "\t},\n";
    out << "\t\"time\": 1,\n";
    out << "\t\"verdict\": \"OK\"\n";
    out << "}";
    return out.str();
}

string playerEntry(int player, const BotRun& run, const Move* response, const string& verdict)
{
    ostringstream out;
    out << "{\n";
    out << "\t\"" << player << "\": {\n";
    out << "\t\t\"keep_running\": " << (run.keepRunning ? "true" : "false") << ",\n";
    out << "\t\t\"memory\": " << run.memoryKb << ",\n";
    out << "\t\t\"time\": " << max<long long>(1, run.timeMs) << ",\n";
    out << "\t\t\"verdict\": \"" << jsonEscape(verdict) << "\",\n";
    out << "\t\t\"raw\": \"" << jsonEscape(trim(run.raw)) << "\",\n";
    out << "\t\t\"debug\": \"" << jsonEscape(trim(run.debug)) << "\"";
    if (response) {
        out << ",\n";
        out << "\t\t\"response\": " << moveObject(*response, 2) << "\n";
    } else {
        out << "\n";
    }
    out << "\t}\n";
    out << "}";
    return out.str();
}

string boolJson(bool value)
{
    return value ? "true" : "false";
}

string gameSummaryEntry(const GameResult& game, int gameIndex, int indent)
{
    ostringstream out;
    out << "{\n";
    out << tabs(indent + 1) << "\"game\": " << gameIndex << ",\n";
    out << tabs(indent + 1) << "\"blackone\": \"" << jsonEscape(game.blackName) << "\",\n";
    out << tabs(indent + 1) << "\"whiteone\": \"" << jsonEscape(game.whiteName) << "\",\n";
    out << tabs(indent + 1) << "\"winner\": \"" << jsonEscape(game.winnerName) << "\",\n";
    out << tabs(indent + 1) << "\"winner_color\": " << game.winner << ",\n";
    out << tabs(indent + 1) << "\"reason\": \"" << jsonEscape(game.reason) << "\",\n";
    out << tabs(indent + 1) << "\"abnormal\": " << boolJson(game.abnormal) << "\n";
    out << tabs(indent) << "}";
    return out.str();
}

bool writeSummaryJson(const vector<GameResult>& games, const Summary& summary,
                      int totalRounds, const fs::path& path)
{
    map<string, int> abnormalBreakdown;
    for (const auto& game : games) {
        if (game.abnormal) {
            ++abnormalBreakdown[game.reason];
        }
    }

    ofstream out(path, ios::binary);
    if (!out) {
        return false;
    }

    out << "{\n";
    out << "\t\"type\": \"arena_summary\",\n";
    out << "\t\"total_games\": " << totalRounds << ",\n";
    out << "\t\"time_limit_ms\": " << TIME_LIMIT_MS << ",\n";
    out << "\t\"players\": [\"alice\", \"james\"],\n";
    out << "\t\"summary\": {\n";
    out << "\t\t\"alice_wins\": " << summary.aliceWins << ",\n";
    out << "\t\t\"james_wins\": " << summary.jamesWins << ",\n";
    out << "\t\t\"ties\": " << summary.ties << ",\n";
    out << "\t\t\"alice_as_black\": " << summary.aliceAsBlack << ",\n";
    out << "\t\t\"james_as_black\": " << summary.jamesAsBlack << ",\n";
    out << "\t\t\"abnormal_games\": " << summary.abnormalGames << "\n";
    out << "\t},\n";
    out << "\t\"abnormal_breakdown\": {\n";
    size_t written = 0;
    for (const auto& item : abnormalBreakdown) {
        out << "\t\t\"" << jsonEscape(item.first) << "\": " << item.second;
        if (++written != abnormalBreakdown.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "\t},\n";
    out << "\t\"games\": [\n";
    for (size_t i = 0; i < games.size(); ++i) {
        out << tabs(2) << gameSummaryEntry(games[i], static_cast<int>(i + 1), 2);
        if (i + 1 != games.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "\t]\n";
    out << "}\n";
    return true;
}

bool writeGameLog(const vector<string>& entries, const fs::path& path);

bool writeCombinedBotzoneLog(const vector<GameResult>& games, const fs::path& path)
{
    vector<string> entries;
    for (const auto& game : games) {
        entries.insert(entries.end(), game.logEntries.begin(), game.logEntries.end());
    }
    return writeGameLog(entries, path);
}

string indentBlock(const string& block, int indent)
{
    istringstream in(block);
    ostringstream out;
    string line;
    bool first = true;
    while (getline(in, line)) {
        if (!first) {
            out << "\n";
        }
        first = false;
        out << tabs(indent) << line;
    }
    return out.str();
}

bool writeGameLog(const vector<string>& entries, const fs::path& path)
{
    ofstream out(path, ios::binary);
    if (!out) {
        return false;
    }
    out << "[\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        out << indentBlock(entries[i], 1);
        if (i + 1 != entries.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "]\n";
    return true;
}

string readWholeFile(const fs::path& path)
{
    ifstream in(path, ios::binary);
    if (!in) {
        return "";
    }
    ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool writeTextFile(const fs::path& path, const string& content)
{
    ofstream out(path, ios::binary);
    if (!out) {
        return false;
    }
    out << content;
    return true;
}

fs::path nextIndexedJsonPath(const fs::path& resultDir, const string& baseName)
{
    fs::create_directories(resultDir);

    const fs::path defaultPath = resultDir / (baseName + ".json");
    if (!fs::exists(defaultPath)) {
        return defaultPath;
    }

    for (int idx = 1; idx < 1000000; ++idx) {
        const fs::path candidate = resultDir / (baseName + "_" + to_string(idx) + ".json");
        if (!fs::exists(candidate)) {
            return candidate;
        }
    }

    return defaultPath;
}

fs::path nextSummaryPath(int rounds)
{
    return nextIndexedJsonPath(fs::path("result"), "summary-" + to_string(rounds));
}

fs::path nextLatestSummaryPath()
{
    return nextIndexedJsonPath(fs::path("result"), "latest_summary");
}

vector<string> compilerCandidates()
{
    vector<string> candidates = {
        "g++",
        "clang++",
#ifdef _WIN32
        "C:\\msys64\\ucrt64\\bin\\g++.exe",
        "C:\\msys64\\mingw64\\bin\\g++.exe",
        "C:\\MinGW\\bin\\g++.exe",
        "C:\\mingw64\\bin\\g++.exe",
        "C:\\Program Files\\LLVM\\bin\\clang++.exe"
#else
        "/usr/bin/g++",
        "/usr/local/bin/g++",
        "/usr/bin/clang++",
        "/usr/local/bin/clang++"
#endif
    };
    return candidates;
}

BotRun runBotProcess(const fs::path& exePath, const string& input, int timeLimitMs,
                     int gameIndex, int actionIndex, int player)
{
    BotRun result;
    cout << "[BOT] Game " << gameIndex << " action " << actionIndex << " player " << player << " starting\n" << flush;
    const fs::path tempDir = ".arena";
    fs::create_directories(tempDir);

#ifdef _WIN32
    const unsigned long processId = static_cast<unsigned long>(GetCurrentProcessId());
#else
    const unsigned long processId = static_cast<unsigned long>(std::chrono::steady_clock::now().time_since_epoch().count());
#endif
    const string stem = "p" + to_string(processId) +
        "_g" + to_string(gameIndex) +
        "_a" + to_string(actionIndex) +
        "_p" + to_string(player);
    const fs::path inputPath = tempDir / (stem + ".in.json");
    const fs::path outputPath = tempDir / (stem + ".out.txt");
    const fs::path errorPath = tempDir / (stem + ".err.txt");

    if (!writeTextFile(inputPath, input)) {
        result.verdict = "RE";
        result.debug = "failed to write bot input";
        return result;
    }

    auto started = chrono::steady_clock::now();

#ifdef _WIN32
    const string ps =
        "$ErrorActionPreference='Stop'; "
        "try { "
        "$env:PATH='C:\\msys64\\ucrt64\\bin;'+$env:PATH; "
        "$p=Start-Process -FilePath " + quoteForPowerShell(fs::absolute(exePath).string()) +
        " -RedirectStandardInput " + quoteForPowerShell(fs::absolute(inputPath).string()) +
        " -RedirectStandardOutput " + quoteForPowerShell(fs::absolute(outputPath).string()) +
        " -RedirectStandardError " + quoteForPowerShell(fs::absolute(errorPath).string()) +
        " -PassThru -WindowStyle Hidden -ErrorAction Stop; "
        "if (-not $p.WaitForExit(" + to_string(timeLimitMs) + ")) { try { $p.Kill() } catch {}; exit 124 }; "
        "exit $p.ExitCode "
        "} catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }";
    string command = "powershell -NoProfile -ExecutionPolicy Bypass -Command " + quoteForCmd(ps);
    int code = system(command.c_str());
    result.exitCode = code;
    if (code == 0) {
        result.verdict = "OK";
    } else if (code == 124 || code == 31744) {
        result.verdict = "TLE";
    } else {
        result.verdict = "RE";
    }
#else
    const int timeoutSec = max(1, (timeLimitMs + 999) / 1000);
    string command = "timeout " + to_string(timeoutSec) + "s " +
        quoteForCmd(fs::absolute(exePath).string()) + " < " +
        quoteForCmd(inputPath.string()) + " > " +
        quoteForCmd(outputPath.string()) + " 2> " +
        quoteForCmd(errorPath.string());
    int code = system(command.c_str());
    result.exitCode = code;
    if (code == 0) {
        result.verdict = "OK";
    } else if (code == 124 || code == 31744) {
        result.verdict = "TLE";
    } else {
        result.verdict = "RE";
    }
#endif

    auto finished = chrono::steady_clock::now();
    result.timeMs = max<long long>(
        1,
        chrono::duration_cast<chrono::milliseconds>(finished - started).count());
#ifdef _WIN32
    result.timeMs = (result.verdict == "TLE")
        ? timeLimitMs
        : min<long long>(result.timeMs, timeLimitMs);
#endif
    result.raw = readWholeFile(outputPath);
    {
        string fileDebug = readWholeFile(errorPath);
        if (!fileDebug.empty()) {
            if (!result.debug.empty()) {
                result.debug += "\n";
            }
            result.debug += fileDebug;
        }
    }

    error_code ec;
    fs::remove(inputPath, ec);
    fs::remove(outputPath, ec);
    fs::remove(errorPath, ec);
    return result;
}

bool compileBot(const fs::path& source, const fs::path& exePath)
{
    if (!fs::exists(source)) {
        cerr << "Source not found: " << source.string() << "\n";
        return false;
    }

    // Python 脚本不需要编译，生成一个 shell wrapper
    if (source.extension() == ".py") {
        fs::create_directories(exePath.parent_path());
        string absSource = fs::absolute(source).string();
        string wrapperContent = "#!/bin/sh\nexec python3 " + quoteForCmd(absSource) + " \"$@\"\n";
        if (!writeTextFile(exePath, wrapperContent)) {
            cerr << "Failed to write python wrapper\n";
            return false;
        }
        fs::permissions(exePath, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                        fs::perm_options::add);
        cout << "Python script: " << source.string() << " -> " << exePath.string() << "\n";
        return true;
    }

    fs::create_directories(exePath.parent_path());
    for (const string& compiler : compilerCandidates()) {
#ifdef _WIN32
        if (compiler.find(' ') != string::npos) {
            continue;
        }
        if (compiler.find(':') != string::npos && !fs::exists(compiler)) {
            continue;
        }
        const string compilerCommand = compiler;
#else
        const string compilerCommand = quoteForCmd(compiler);
#endif
        vector<string> compileFlags = {" -I/usr/include/jsoncpp -ljsoncpp", " -I/usr/include/jsoncpp", " -ljsoncpp", ""};
        for (const string& flags : compileFlags) {
            string command = compilerCommand + " -std=c++17 -O2 -o " +
                quoteForCmd(exePath.string()) + " " + quoteForCmd(source.string()) + flags;
            cout << "Compiling " << source.string() << " with " << compiler;
            if (!flags.empty()) {
                cout << " and" << flags;
            }
            cout << "...\n";
            int code = system(command.c_str());
            if (code == 0 && fs::exists(exePath)) {
                return true;
            }
        }
    }
    return false;
}

GameResult playOneGame(int gameIndex, int totalRounds, const fs::path& blackExe,
                       const fs::path& whiteExe, const string& blackName,
                       const string& whiteName, int blackTimeLimitMs, int whiteTimeLimitMs)
{
    resetBoard();

    vector<Move> requests[2];
    vector<Move> responses[2];
    vector<string> entries;
    entries.push_back(matchInfoEntry(gameIndex, totalRounds, blackName, whiteName));
    requests[PLAYER_BLACK].push_back(Move{ -1, -1, -1, -1 });
    entries.push_back(judgeRequestEntry(PLAYER_BLACK, requests[PLAYER_BLACK].back(), false));

    int currentPlayer = PLAYER_BLACK;
    int actionIndex = 0;
    GameResult game;
    game.blackName = blackName;
    game.whiteName = whiteName;

    while (true) {
        const fs::path& exe = currentPlayer == PLAYER_BLACK ? blackExe : whiteExe;
        const int timeLimitMs = currentPlayer == PLAYER_BLACK ? blackTimeLimitMs : whiteTimeLimitMs;
        string botInput = makeBotInput(entries, requests[currentPlayer], responses[currentPlayer], currentPlayer);
        BotRun run = runBotProcess(exe, botInput, timeLimitMs, gameIndex, actionIndex, currentPlayer);

        Move mv;
        bool hasParsedMove = false;
        string botVerdict = run.verdict;
        if (run.verdict == "OK") {
            hasParsedMove = parseMove(run.raw, mv);
            if (!hasParsedMove) {
                botVerdict = "INVALID_OUTPUT";
            }
        }
        entries.push_back(playerEntry(currentPlayer, run, hasParsedMove ? &mv : nullptr, botVerdict));

        if (botVerdict != "OK") {
            const int winner = 1 - currentPlayer;
            string err = "INVALID_INPUT_VERDICT_" + botVerdict;
            entries.push_back(judgeFinishEntry(winner, err, nullptr));
            game.winner = winner;
            game.winnerName = winner == PLAYER_BLACK ? blackName : whiteName;
            game.abnormal = true;
            game.reason = err;
            break;
        }

        const bool firstMove = actionIndex == 0;
        if (!isLegalMove(mv, currentPlayer, firstMove)) {
            const int winner = 1 - currentPlayer;
            entries.push_back(judgeFinishEntry(winner, "INVALIDMOVE", nullptr));
            game.winner = winner;
            game.winnerName = winner == PLAYER_BLACK ? blackName : whiteName;
            game.abnormal = true;
            game.reason = "INVALIDMOVE";
            break;
        }

        applyMove(mv, currentPlayer, firstMove);
        responses[currentPlayer].push_back(mv);

        if (moveMakesWin(mv, currentPlayer, firstMove)) {
            entries.push_back(judgeFinishEntry(currentPlayer, "", &mv));
            game.winner = currentPlayer;
            game.winnerName = currentPlayer == PLAYER_BLACK ? blackName : whiteName;
            game.reason = "WIN";
            break;
        }

        if (countEmptyCells() < 2) {
            entries.push_back(judgeFinishEntry(PLAYER_TIE, "", &mv));
            game.winner = PLAYER_TIE;
            game.winnerName = "tie";
            game.reason = "TIE";
            break;
        }

        const int nextPlayer = 1 - currentPlayer;
        requests[nextPlayer].push_back(mv);
        entries.push_back(judgeRequestEntry(nextPlayer, mv, true));
        currentPlayer = nextPlayer;
        ++actionIndex;

        if (actionIndex > 200) {
            entries.push_back(judgeFinishEntry(PLAYER_TIE, "SAFETY_STOP", nullptr));
            game.winner = PLAYER_TIE;
            game.winnerName = "tie";
            game.abnormal = true;
            game.reason = "SAFETY_STOP";
            break;
        }
    }

    game.logEntries = std::move(entries);
    return game;
}

bool parseIntArg(const string& s, int& value)
{
    try {
        size_t used = 0;
        int parsed = stoi(s, &used);
        if (used != s.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool looksLikeExePath(const string& s)
{
    string lower = s;
    transform(lower.begin(), lower.end(), lower.begin(),
              [](unsigned char ch) { return static_cast<char>(tolower(ch)); });
    return lower.size() >= 4 && lower.substr(lower.size() - 4) == ".exe";
}

void printUsage(const char* program)
{
    cout << "Usage:\n";
    cout << "  " << program << " <rounds> [--no-compile] [--alice-source PATH] [--james-source PATH]\n";
    cout << "\n";
    cout << "Arena players:\n";
    cout << "  alice.cpp and james.cpp swap black/white every game by default.\n";
    cout << "  Use --alice-source / --james-source to swap in other bot files.\n";
    cout << "  Per-move time limit is fixed at 1000 ms.\n";
    cout << "\n";
    cout << "Bot protocol:\n";
    cout << "  stdin : one Botzone-like JSON line with requests/responses arrays\n";
    cout << "  stdout: x0 y0 x1 y1  (or JSON object with x0,y0,x1,y1)\n";
}

int main(int argc, char* argv[])
{
    int rounds = 0;
    int aliceTimeLimitMs = TIME_LIMIT_MS;   // alice严格1s
    int jamesTimeLimitMs = TIME_LIMIT_MS;   // james默认1s，可用--james-time覆盖
    bool noCompile = false;
    fs::path aliceSrc = "alice.cpp";
    fs::path jamesSrc = "james.cpp";

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (looksLikeExePath(arg)) {
            continue;
        }
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--no-compile") {
            noCompile = true;
        } else if (arg == "--time") {
            cerr << "The per-move time limit is fixed at 1000 ms; --time is disabled.\n";
            return 1;
        } else if (arg == "--james-time" && i + 1 < argc) {
            if (!parseIntArg(argv[++i], jamesTimeLimitMs) || jamesTimeLimitMs <= 0) {
                cerr << "Invalid james-time value.\n";
                return 1;
            }
        } else if (arg == "--alice-source" && i + 1 < argc) {
            aliceSrc = argv[++i];
        } else if (arg == "--james-source" && i + 1 < argc) {
            jamesSrc = argv[++i];
        } else if ((arg == "--rounds" || arg == "-n") && i + 1 < argc) {
            if (!parseIntArg(argv[++i], rounds) || rounds <= 0) {
                cerr << "Invalid rounds value.\n";
                return 1;
            }
        } else if (rounds == 0) {
            if (!parseIntArg(arg, rounds) || rounds <= 0) {
                cerr << "Invalid rounds value: " << arg << "\n";
                return 1;
            }
        } else {
            cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (rounds <= 0) {
        cout << "Rounds: ";
        if (!(cin >> rounds) || rounds <= 0) {
            cerr << "Invalid rounds value.\n";
            return 1;
        }
    }

    fs::create_directories(".arena");
    fs::create_directories("result");

#ifdef _WIN32
    const string exeSuffix = ".exe";
#else
    const string exeSuffix = "";
#endif

    const fs::path aliceExe = fs::path(".arena") / ("a" + exeSuffix);
    const fs::path jamesExe = fs::path(".arena") / ("j" + exeSuffix);

    if (!noCompile) {
        if (!compileBot(aliceSrc, aliceExe)) {
            cerr << "Failed to compile alice. Checked source: "
                 << aliceSrc.string() << "\n";
            return 1;
        }
        if (!compileBot(jamesSrc, jamesExe)) {
            cerr << "Failed to compile james. Checked source: "
                 << jamesSrc.string() << "\n";
            return 1;
        }
    } else {
        if (!fs::exists(aliceExe) || !fs::exists(jamesExe)) {
            cerr << "--no-compile needs existing executables:\n";
            cerr << "  " << aliceExe.string() << "\n";
            cerr << "  " << jamesExe.string() << "\n";
            return 1;
        }
    }

    Summary summary;
    vector<GameResult> games;
    games.reserve(rounds);

    auto globalStart = chrono::steady_clock::now();
    cout << "Starting " << rounds << " game(s), alice time limit "
         << aliceTimeLimitMs << " ms, james time limit "
         << jamesTimeLimitMs << " ms per move.\n";
    cout << "Alice source: " << aliceSrc.string() << "\n";
    cout << "James source: " << jamesSrc.string() << "\n";
    if (rounds % 2 == 1) {
        cout << "Odd game count: alice will be black one more time than james.\n";
    }

    for (int i = 1; i <= rounds; ++i) {
        auto gameStart = chrono::steady_clock::now();
        const bool aliceBlack = (i % 2 == 1);
        const fs::path& blackExe = aliceBlack ? aliceExe : jamesExe;
        const fs::path& whiteExe = aliceBlack ? jamesExe : aliceExe;
        const string blackName = aliceBlack ? "alice" : "james";
        const string whiteName = aliceBlack ? "james" : "alice";
        const int blackTimeMs = aliceBlack ? aliceTimeLimitMs : jamesTimeLimitMs;
        const int whiteTimeMs = aliceBlack ? jamesTimeLimitMs : aliceTimeLimitMs;

        if (aliceBlack) {
            ++summary.aliceAsBlack;
        } else {
            ++summary.jamesAsBlack;
        }

        cout << flush;
        cout << "[DEBUG] Game " << i << " starting (" << blackName << " vs " << whiteName << ")\n" << flush;
        GameResult result = playOneGame(i, rounds, blackExe, whiteExe,
                                        blackName, whiteName, blackTimeMs, whiteTimeMs);
        cout << "[DEBUG] Game " << i << " finished\n" << flush;
        games.push_back(result);

        if (result.winnerName == "alice") {
            ++summary.aliceWins;
        } else if (result.winnerName == "james") {
            ++summary.jamesWins;
        } else {
            ++summary.ties;
        }
        if (result.abnormal) {
            ++summary.abnormalGames;
        }

        auto gameEnd = chrono::steady_clock::now();
        long long gameMs = chrono::duration_cast<chrono::milliseconds>(gameEnd - gameStart).count();
        cout << "Game " << i << "/" << rounds << " [" << gameMs << "ms]: ";
        cout << "blackone=" << blackName << ", whiteone=" << whiteName << " -> ";
        if (result.winnerName == "alice" || result.winnerName == "james") {
            cout << result.winnerName << " wins";
        } else {
            cout << "tie";
        }
        cout << " (" << result.reason << ")\n";
    }

    const fs::path summaryPath = nextSummaryPath(rounds);
    const fs::path latestSummaryPath = nextLatestSummaryPath();
    if (!writeCombinedBotzoneLog(games, summaryPath)) {
        cerr << "Failed to write combined log: " << summaryPath.string() << "\n";
    }
    if (!writeCombinedBotzoneLog(games, latestSummaryPath)) {
        cerr << "Failed to write combined log: " << latestSummaryPath.string() << "\n";
    }

    cout << "\nSummary\n";
    cout << "  alice wins    : " << summary.aliceWins << "\n";
    cout << "  james wins    : " << summary.jamesWins << "\n";
    cout << "  ties          : " << summary.ties << "\n";
    cout << "  alice as black: " << summary.aliceAsBlack << "\n";
    cout << "  james as black: " << summary.jamesAsBlack << "\n";
    cout << "  abnormal games: " << summary.abnormalGames << "\n";
    cout << "  summary json  : " << summaryPath.string() << "\n";
    cout << "  result folder : " << fs::absolute("result").string() << "\n";

    return 0;
}
