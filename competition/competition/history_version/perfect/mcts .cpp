#ifdef _BOTZONE_ONLINE
#include "jsoncpp/json.h"
#else
#include <json/json.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

typedef long long ll;

class ConnectSix
{
   public:
    static const int N = 15;
    static const int M = 15;
    static const ll WIN = 1e18;
    static const int DEPTH = 12;
    static constexpr int BREADTH[DEPTH + 1] = {2, 3, 3, 4, 4, 5, 5, 6, 6, 8, 8, 12, 12};
    static constexpr double C = 1.5;

    // 连续零威胁，连续一威胁，连续二威胁
    static constexpr ll WEIGHT[6][3] = {{1, 1, 1},          // 零
                                        {1, 1, 1},          // 一
                                        {1, 1, 2},          // 二
                                        {1, 3, 8},          // 三
                                        {1, 100, 10000},    // 四
                                        {1, 1000, 10050}};  // 五
    // 零威胁夹一威胁，一威胁夹一威胁，二威胁夹一威胁
    static constexpr ll HOPWEIGHT[6][3] = {{1, 1, 1},          // 零
                                           {1, 1, 1},          // 一
                                           {1, 1, 1},          // 二
                                           {1, 2, 5},          // 三
                                           {1, 110, 120},      // 四
                                           {900, 950, 1050}};  // 五

   public:
    enum Player {
        Self = 0,
        Opp = 1,
        Blank = 2,
        Out = 3,
    };

    struct Coor
    {
        int x, y;

        Coor(int x = -1, int y = -1) : x(x), y(y)
        {}

        Coor operator+(const Coor &rhs) const
        {
            return {x + rhs.x, y + rhs.y};
        }
        Coor operator-(const Coor &rhs) const
        {
            return {x - rhs.x, y - rhs.y};
        }
        Coor operator*(int rhs) const
        {
            return {x * rhs, y * rhs};
        }
        friend Coor operator*(int lhs, const Coor &rhs)
        {
            return {lhs * rhs.x, lhs * rhs.y};
        }
        bool operator==(const Coor &rhs) const
        {
            return x == rhs.x && y == rhs.y;
        }
        bool operator!=(const Coor &rhs) const
        {
            return !(operator==(rhs));
        }
        bool operator<(const Coor &rhs) const
        {
            return x == rhs.x ? y < rhs.y : x < rhs.x;
        }
    };

    struct Move
    {
        Coor c;
        ll w1;
        int w2;

        Move(ConnectSix *game, Coor coor)
            : Move(coor, max(game->calc(coor, Self), game->calc(coor, Opp)))
        {}

        Move(Coor c, ll w) : c(c), w1(w), w2((abs(c.x - N / 2) + 1) * (abs(c.y - M / 2) + 1))
        {}

        bool operator<(const Move &rhs) const
        {
            if (w1 != rhs.w1)
                return w1 > rhs.w1;
            if (w2 != rhs.w2)
                return w2 < rhs.w2;
            return c < rhs.c;
        }
    };

   private:
    static const Coor DIR[4];

    struct Status  // the status of a cell, assuming that the cell is owned by *player*
    {
        Player player;
        // array index: *direction*, 0 for left, 1 for right
        array<int, 2> len;     // the first *len* cells in the *direction* are *player*
        array<bool, 2> blank;  // whether the (*len*+1)-th cell in the *direction* is *Blank*
        array<int, 2> hopLen;  // except for a single cell which can be *Blank*, the first *hopLen*
                               // cells in the *direction* are *player*
        array<bool, 2> hopBlank;  // whether the (*hopLen*+1)-th cell in the *direction* is *Blank*

        Status()
        {
            len.fill(0);
            blank.fill(false);
            hopLen.fill(0);
            hopBlank.fill(false);
        }

        void update(const Status &s, Player sp, int lr)
        {
            if (sp == Blank)
            {
                len[lr] = 0;
                blank[lr] = true;
                hopLen[lr] = s.len[lr] + 1;
                hopBlank[lr] = s.blank[lr];
            }
            else if (sp == player)
            {
                len[lr] = s.len[lr] + 1;
                blank[lr] = s.blank[lr];
                hopLen[lr] = s.hopLen[lr] + 1;
                hopBlank[lr] = s.hopBlank[lr];
            }
            else if (sp == (player ^ 1))
            {
                len[lr] = hopLen[lr] = 0;
                blank[lr] = hopBlank[lr] = false;
            }
        }

        ll weight() const
        {
            const int totalLen = len[0] + len[1] + 1;
            if (totalLen >= 6)
                return WIN;
            const int lHopLen = hopLen[0] + len[1];
            const int rHopLen = len[0] + hopLen[1];
            return max({WEIGHT[totalLen][blank[0] + blank[1]],
                        HOPWEIGHT[min(5, lHopLen)][hopBlank[0] + blank[1]],
                        HOPWEIGHT[min(5, rHopLen)][blank[0] + hopBlank[1]]});
        }
    };

   private:
    static bool inGrid(Coor coor)
    {
        return coor.x >= 0 && coor.x < N && coor.y >= 0 && coor.y < M;
    }

    array<Status, 2> &s(Coor coor, int direction)
    {
        return status[coor.x][coor.y][direction];
    }

    Player c(Coor coor) const
    {
        if (!inGrid(coor))
            return Out;
        return grid[coor.x][coor.y];
    }

    bool timeout() const
    {
        using namespace chrono;
        return duration_cast<milliseconds>(steady_clock::now() - start).count() > 920;
    }

   private:
    array<array<Player, M>, N> grid;
    array<array<array<array<Status, 2>, 4>, M>, N> status;
    array<array<array<ll, 2>, M>, N> calcResult;
    set<Move> moves;
    chrono::time_point<chrono::steady_clock> start;

   public:
    explicit ConnectSix(chrono::time_point<chrono::steady_clock> t = chrono::steady_clock::now())
        : start(t)
    {
        for (int x = 0; x < N; ++x)
        {
            for (int y = 0; y < M; ++y)
            {
                grid[x][y] = Blank;
                const Coor u(x, y);
                for (int i = 0; i < 4; ++i)
                {
                    auto &cur = s(u, i);
                    cur[0].player = (Player)0;
                    cur[1].player = (Player)1;
                    if (inGrid(u - DIR[i]))
                    {
                        cur[0].blank[0] = cur[1].blank[0] = true;
                        cur[0].hopLen[0] = cur[1].hopLen[0] = 1;
                        if (inGrid(u - DIR[i] * 2))
                            cur[0].hopBlank[0] = cur[1].hopBlank[0] = true;
                    }
                    if (inGrid(u + DIR[i]))
                    {
                        cur[0].blank[1] = cur[1].blank[1] = true;
                        cur[0].hopLen[1] = cur[1].hopLen[1] = 1;
                        if (inGrid(u + DIR[i] * 2))
                            cur[0].hopBlank[1] = cur[1].hopBlank[1] = true;
                    }
                }
                reCalc(u, Self);
                reCalc(u, Opp);
                moves.emplace(this, u);
            }
        }
    }

    void reCalc(Coor coor, Player player)
    {
        ll res = 1;
        for (int i = 0; i < 4; ++i)
        {
            const ll weight = status[coor.x][coor.y][i][player].weight();
            if (weight == WIN)
            {
                res = WIN;
                break;
            }
            res *= weight;
        }
        calcResult[coor.x][coor.y][player] = res;
    }

    ll calc(Coor coor, Player player) const
    {
        return calcResult[coor.x][coor.y][player];
    }

    void modify(Coor u, Player player)
    {
        if (u.x == -1)
            return;

        if (player == c(u))
            return;

        if (grid[u.x][u.y] == Blank)
            moves.erase({this, u});

        grid[u.x][u.y] = player;

        static vector<vector<int>> changed(N, vector<int>(M));
        static int changedTim = 0;
        ++changedTim;
        vector<Move> changedList;

        for (int i = 0; i < 4; ++i)
        {
            for (int p = 0; p < 2; ++p)
            {
                const auto cur = s(u, i)[p];
                for (int lr = 0; lr <= 1; ++lr)
                {
                    const auto d = lr ? DIR[i] : Coor(-DIR[i].x, -DIR[i].y);
                    const auto forTo = u + (cur.hopLen[lr] + cur.hopBlank[lr] + 1) * d;
                    for (auto v = u + d; v != forTo; v = v + d)
                    {
                        if (c(v) == Blank && changed[v.x][v.y] != changedTim)
                        {
                            changed[v.x][v.y] = changedTim;
                            changedList.emplace_back(this, v);
                        }
                        s(v, i)[p].update(s(v - d, i)[p], c(v - d), lr ^ 1);
                    }
                }
            }
        }

        for (const auto &v : changedList)
            moves.erase(v);
        for (const auto &v : changedList)
        {
            reCalc(v.c, Self);
            reCalc(v.c, Opp);
            moves.emplace(this, v.c);
        }
        if (player == Blank)
            moves.emplace(this, u);
    }

    struct Node
    {
        int visit = 0;
        int win = 0;
        Player player;
        Player end;
        Move move1;
        Move move2;
        Node *parent;
        vector<Node *> children;

        Node(Player player, Move move1, Move move2, Node *parent, Player end = Blank)
            : player(player),
              end(end),
              move1(std::move(move1)),
              move2(std::move(move2)),
              parent(parent)
        {}

        void update(Player winner)
        {
            visit += 2;
            if (winner == (player ^ 1))
                win += 2;
            else if (winner == Blank)
                ++win;
        }

        double uct() const
        {
            return (double)win / visit + C * sqrt(log(parent->visit) / visit);
        }
    };

    Player mcts(Node *u, int depth = DEPTH)
    {
        if (u->visit == 0)
        {
            if (u->end == Blank && depth > 0)
            {
                const ll minw = min(1000.0, sqrt(moves.begin()->w1));
                vector<Move> moves1;
                for (const auto &move : moves)
                {
                    if ((int)moves1.size() >= max(2, BREADTH[depth] / 2) &&
                        (move.w1 < minw || (int)moves1.size() >= BREADTH[depth]))
                        break;
                    moves1.emplace_back(move);
                }
                set<pair<Coor, Coor>> vis;
                for (int i = 0; i < (int)moves1.size(); ++i)
                {
                    const auto move1 = moves1[i];
                    if (calc(move1.c, u->player) == WIN)
                    {
                        u->children = {new Node(Player(u->player ^ 1), move1,
                                                moves1[i == 0 ? 1 : i - 1], u, u->player)};
                        break;
                    }
                    int cnt = 0;
                    bool win = false;
                    modify(move1.c, u->player);
                    for (const auto &move2 : moves)
                    {
                        if (!vis.insert({min(move1.c, move2.c), max(move1.c, move2.c)}).second)
                            continue;
                        if (++cnt > (BREADTH[depth] - i) / 2 + 1)
                            break;
                        if (calc(move2.c, u->player) == WIN)
                        {
                            win = true;
                            u->children = {
                                new Node(Player(u->player ^ 1), move1, move2, u, u->player)};
                            break;
                        }
                        u->children.push_back(new Node(Player(u->player ^ 1), move1, move2, u));
                    }
                    modify(move1.c, Blank);
                    if (win)
                        break;
                }
            }
        }

        if (u->children.empty())
        {
            u->update(u->end);
            return u->end;
        }

        double mx = -1;
        Node *choose = nullptr;
        for (auto v : u->children)
        {
            if (v->visit == 0)
            {
                choose = v;
                break;
            }
            if (v->uct() > mx)
            {
                mx = v->uct();
                choose = v;
            }
        }

        modify(choose->move1.c, u->player);
        modify(choose->move2.c, u->player);
        const auto res = mcts(choose, depth - 1);
        modify(choose->move2.c, Blank);
        modify(choose->move1.c, Blank);
        u->update(res);
        return res;
    }

    Json::Value mcts()
    {
        auto *root = new Node(Self, {{-1, -1}, 0}, {{-1, -1}, 0}, nullptr);

        while (!timeout())
            mcts(root);

        const auto best =
            *max_element(root->children.begin(), root->children.end(), [](Node *lhs, Node *rhs) {
                return lhs->visit < rhs->visit;
            });

        ostringstream debug;
        for (auto v : root->children)
        {
            debug << v->move1.c.x << ',' << v->move1.c.y << ' ' << v->move1.w1 << ' ';
            debug << v->move2.c.x << ',' << v->move2.c.y << ' ' << v->move2.w1 << ' ';
            debug << v->win << '/' << v->visit - v->win << "    ";
        }

        Json::Value output;
        output["response"]["x0"] = best->move1.c.x;
        output["response"]["y0"] = best->move1.c.y;
        output["response"]["x1"] = best->move2.c.x;
        output["response"]["y1"] = best->move2.c.y;
        output["debug"] = debug.str();

        return output;
    }
};

const ConnectSix::Coor ConnectSix::DIR[4] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

int main()
{
    auto programStart = chrono::steady_clock::now();
    string str((istreambuf_iterator<char>(cin)), istreambuf_iterator<char>());
    Json::Value input;
    Json::Reader reader;
    if (!reader.parse(str, input))
    {
        Json::Value output;
        output["response"]["x0"] = ConnectSix::N / 2;
        output["response"]["y0"] = ConnectSix::M / 2;
        output["response"]["x1"] = -1;
        output["response"]["y1"] = -1;
        cout << Json::FastWriter().write(output) << endl;
        return 0;
    }
    const int turnID = input["requests"].size();
    const bool isBlack = input["requests"][0u]["x0"].asInt() == -1;
    ConnectSix game(programStart);
    for (int i = 0; i < turnID; i++)
    {
        game.modify({input["requests"][i]["x0"].asInt(), input["requests"][i]["y0"].asInt()},
                    ConnectSix::Opp);
        game.modify({input["requests"][i]["x1"].asInt(), input["requests"][i]["y1"].asInt()},
                    ConnectSix::Opp);
        if (i == turnID - 1)
            break;
        game.modify({input["responses"][i]["x0"].asInt(), input["responses"][i]["y0"].asInt()},
                    ConnectSix::Self);
        game.modify({input["responses"][i]["x1"].asInt(), input["responses"][i]["y1"].asInt()},
                    ConnectSix::Self);
    }

    Json::Value output;

    if (turnID == 1 && isBlack)
    {
        output["response"]["x0"] = ConnectSix::N / 2;
        output["response"]["y0"] = ConnectSix::M / 2;
        output["response"]["x1"] = -1;
        output["response"]["y1"] = -1;
    }
    else
        output = game.mcts();

    cout << Json::FastWriter().write(output) << endl;

    return 0;
}