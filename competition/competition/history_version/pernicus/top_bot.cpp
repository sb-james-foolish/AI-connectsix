#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cassert>
#include <climits>
using namespace std;

// ============================================================
//  常量
// ============================================================
static const int N = 19;
static const int CELLS = N * N; // 361
static const int BLACK = 0;
static const int WHITE = 1;
static const int EMPTY = 2;

static const int WIN_SCORE  =  1000000000;
static const int LOSE_SCORE = -1000000000;
static const int INF        =  1100000000;

// ============================================================
//  计时
// ============================================================
static chrono::steady_clock::time_point g_start;
static double g_time_limit = 880.0;

inline double elapsed_ms() {
    return chrono::duration<double, milli>(
        chrono::steady_clock::now() - g_start).count();
}
inline bool timeUp() { return elapsed_ms() >= g_time_limit; }

// ============================================================
//  位棋盘  (361位 = 6×uint64)
// ============================================================
struct Bitboard {
    uint64_t w[6]{};
    inline void set(int i)  { w[i>>6] |=  (1ULL<<(i&63)); }
    inline void clr(int i)  { w[i>>6] &= ~(1ULL<<(i&63)); }
    inline bool get(int i) const { return (w[i>>6]>>(i&63))&1; }
    inline bool any() const { return w[0]|w[1]|w[2]|w[3]|w[4]|w[5]; }
    Bitboard operator|(const Bitboard& o) const {
        Bitboard r; for(int i=0;i<6;i++) r.w[i]=w[i]|o.w[i]; return r;
    }
    Bitboard operator&(const Bitboard& o) const {
        Bitboard r; for(int i=0;i<6;i++) r.w[i]=w[i]&o.w[i]; return r;
    }
    Bitboard andnot(const Bitboard& o) const {
        Bitboard r; for(int i=0;i<6;i++) r.w[i]=w[i]&~o.w[i]; return r;
    }
    // 遍历所有置位
    template<typename F> void forEach(F f) const {
        for(int k=0;k<6;k++){
            uint64_t tmp=w[k];
            while(tmp){ int b=__builtin_ctzll(tmp); f(k*64+b); tmp&=tmp-1; }
        }
    }
    int count() const {
        int c=0; for(int k=0;k<6;k++) c+=__builtin_popcountll(w[k]); return c;
    }
};

// ============================================================
//  Zobrist
// ============================================================
static uint64_t ZOBRIST[CELLS][2];
static uint64_t ZOBRIST_TURN; // 轮到 WHITE 时 XOR

static void initZobrist() {
    uint64_t s = 0xdeadbeefcafe1234ULL;
    auto rng = [&]()->uint64_t {
        s^=s<<13; s^=s>>7; s^=s<<17; return s;
    };
    for(int i=0;i<CELLS;i++) { ZOBRIST[i][0]=rng(); ZOBRIST[i][1]=rng(); }
    ZOBRIST_TURN = rng();
}

// ============================================================
//  棋盘状态
// ============================================================
struct Board {
    Bitboard bb[2]; // bb[BLACK], bb[WHITE]
    uint64_t zobrist = 0;
    int move_cnt = 0; // 已落子总数

    inline bool occupied(int i) const { return bb[0].get(i)||bb[1].get(i); }
    inline int  colorAt(int i)  const {
        if(bb[0].get(i)) return BLACK;
        if(bb[1].get(i)) return WHITE;
        return EMPTY;
    }
    inline void place(int i, int c) {
        bb[c].set(i);
        zobrist ^= ZOBRIST[i][c];
        move_cnt++;
    }
    inline void remove(int i, int c) {
        bb[c].clr(i);
        zobrist ^= ZOBRIST[i][c];
        move_cnt--;
    }
};

// ============================================================
//  方向定义
// ============================================================
// 4方向：横/竖/斜45/斜135
static const int DX[4] = {0, 1, 1,  1};
static const int DY[4] = {1, 0, 1, -1};

inline int rc2i(int r, int c) { return r*N+c; }
inline int i2r(int i) { return i/N; }
inline int i2c(int i) { return i%N; }
inline bool inBoard(int r, int c) { return (unsigned)r<(unsigned)N && (unsigned)c<(unsigned)N; }

// ============================================================
//  预计算：邻居掩码（距离≤2）
// ============================================================
static Bitboard NEIGHBOR2[CELLS];

static void initNeighbor2() {
    for(int i=0;i<CELLS;i++){
        int r=i2r(i), c=i2c(i);
        for(int dr=-2;dr<=2;dr++)
            for(int dc=-2;dc<=2;dc++){
                int nr=r+dr, nc=c+dc;
                if(inBoard(nr,nc) && !(dr==0&&dc==0))
                    NEIGHBOR2[i].set(rc2i(nr,nc));
            }
    }
}

// ============================================================
//  评估函数（无重复计数）
// ============================================================
// 模式分值
static const int S_SIX        = 1000000000;
static const int S_OPEN_FIVE  =    500000;
static const int S_FIVE       =    100000;
static const int S_OPEN_FOUR  =     50000;
static const int S_FOUR       =     10000;
static const int S_OPEN_THREE =      2000;
static const int S_THREE      =       500;
static const int S_OPEN_TWO   =       100;
static const int S_TWO        =        20;

// 扫描一条线（从起点沿方向d），统计 color 的得分
// 只从"线的起点"扫描，避免重复
static int scoreLine(const Board& b, int r0, int c0, int d, int color) {
    int opp = 1-color;
    int score = 0;
    int r = r0, c = c0;
    // 沿方向扫描，提取连续段
    while(inBoard(r,c)) {
        if(b.colorAt(rc2i(r,c)) == opp) {
            // 对方棋子，跳过
            r+=DX[d]; c+=DY[d]; continue;
        }
        if(b.colorAt(rc2i(r,c)) == EMPTY) {
            r+=DX[d]; c+=DY[d]; continue;
        }
        // 找到 color 的棋子，统计连续段
        int cnt = 0;
        int sr = r, sc_start = c; // 用 sc_start 避免与外层变量名冲突
        while(inBoard(r,c) && b.colorAt(rc2i(r,c))==color) {
            cnt++; r+=DX[d]; c+=DY[d];
        }
        // 检查两端
        int open = 0;
        // 前端
        int pr = sr-DX[d], pc = sc_start-DY[d];
        if(inBoard(pr,pc) && b.colorAt(rc2i(pr,pc))==EMPTY) open++;
        // 后端（r,c）
        if(inBoard(r,c) && b.colorAt(rc2i(r,c))==EMPTY) open++;

        if(cnt>=6) { score += S_SIX; continue; }
        switch(cnt){
            case 5: score += (open>=1 ? S_OPEN_FIVE : S_FIVE); break;
            case 4: score += (open==2 ? S_OPEN_FOUR : (open==1 ? S_FOUR : 0)); break;
            case 3: score += (open==2 ? S_OPEN_THREE : (open==1 ? S_THREE : 0)); break;
            case 2: score += (open==2 ? S_OPEN_TWO : (open==1 ? S_TWO : 0)); break;
            case 1: score += (open==2 ? 5 : 0); break;
        }
    }
    return score;
}

// 全局评估（从 color 视角）
static int evalBoard(const Board& b, int color) {
    int sc[2] = {0,0};
    for(int col=0;col<2;col++){
        for(int d=0;d<4;d++){
            // 枚举每条线的起点（该方向上最左/最上的点）
            for(int r=0;r<N;r++){
                for(int c=0;c<N;c++){
                    // 只从"线起点"开始扫描
                    int pr=r-DX[d], pc=c-DY[d];
                    if(inBoard(pr,pc)) continue; // 不是起点
                    sc[col] += scoreLine(b,r,c,d,col);
                }
            }
        }
    }
    if(sc[color]   >= S_SIX) return  WIN_SCORE;
    if(sc[1-color] >= S_SIX) return LOSE_SCORE;
    return sc[color] - sc[1-color]*2;
}

// 快速单点评估（用于候选排序）
// 注意：只统计经过该点的4条线，不是全局扫描
static int quickEval(const Board& b, int cell, int color) {
    int r=i2r(cell), c=i2c(cell);
    Board& bm = const_cast<Board&>(b);
    bm.bb[color].set(cell); // 不更新 zobrist，只是临时评估
    int sc = 0;
    for(int d=0;d<4;d++){
        int sr=r, sc2=c;
        while(inBoard(sr-DX[d], sc2-DY[d])) { sr-=DX[d]; sc2-=DY[d]; }
        sc += scoreLine(b, sr, sc2, d, color);
    }
    bm.bb[color].clr(cell);
    return sc;
}

// ============================================================
//  胜负检测
// ============================================================
static bool checkWin(const Board& b, int color) {
    for(int r=0;r<N;r++){
        for(int c=0;c<N;c++){
            if(b.colorAt(rc2i(r,c)) != color) continue;
            for(int d=0;d<4;d++){
                int pr=r-DX[d], pc=c-DY[d];
                if(inBoard(pr,pc) && b.colorAt(rc2i(pr,pc))==color) continue;
                int cnt=0, nr=r, nc=c;
                while(inBoard(nr,nc) && b.colorAt(rc2i(nr,nc))==color){
                    cnt++; nr+=DX[d]; nc+=DY[d];
                    if(cnt>=6) return true;
                }
            }
        }
    }
    return false;
}

static bool winsFromPoint(const Board& b, int cell, int color) {
    int r = i2r(cell), c = i2c(cell);
    for(int d=0; d<4; d++) {
        int cnt = 1;
        int nr = r + DX[d], nc = c + DY[d];
        while(inBoard(nr,nc) && b.colorAt(rc2i(nr,nc)) == color) {
            cnt++; nr += DX[d]; nc += DY[d];
        }
        nr = r - DX[d]; nc = c - DY[d];
        while(inBoard(nr,nc) && b.colorAt(rc2i(nr,nc)) == color) {
            cnt++; nr -= DX[d]; nc -= DY[d];
        }
        if(cnt >= 6) return true;
    }
    return false;
}

// ============================================================
//  候选点生成
// ============================================================
struct Cand { int cell, score; };

static vector<Cand> genCandidates(const Board& b, int color) {
    Bitboard occ = b.bb[0]|b.bb[1];
    Bitboard cands;
    occ.forEach([&](int i){ cands = cands | NEIGHBOR2[i]; });
    cands = cands.andnot(occ);

    if(!occ.any()) { // 空棋盘
        vector<Cand> res; res.push_back({rc2i(9,9), 0}); return res;
    }

    vector<Cand> res;
    res.reserve(64);
    int opp = 1-color;
    cands.forEach([&](int i){
        if(i >= CELLS) return;
        int sc = quickEval(b,i,color)*2 + quickEval(b,i,opp);
        res.push_back({i, sc});
    });
    sort(res.begin(), res.end(), [](const Cand& a, const Cand& b){ return a.score > b.score; });
    return res;
}

// 走法对
struct Move2 { int m1, m2, score; };

static vector<Move2> genMovePairs(const Board& b, int color, int maxN) {
    auto cands = genCandidates(b, color);
    int n = min((int)cands.size(), maxN);
    vector<Move2> moves;
    moves.reserve(n*n/2);
    for(int i=0;i<n;i++)
        for(int j=i+1;j<n;j++)
            moves.push_back({cands[i].cell, cands[j].cell, cands[i].score+cands[j].score});
    sort(moves.begin(), moves.end(), [](const Move2& a, const Move2& b){ return a.score > b.score; });
    return moves;
}

// ============================================================
//  置换表
// ============================================================
static const int TT_SIZE = 1<<22; // 4M 条，64MB
struct TTEntry {
    uint64_t key;
    int32_t  score;
    int16_t  depth;
    uint8_t  flag;  // 0=EXACT 1=LOWER 2=UPPER
    uint8_t  pad;
    int32_t  best;  // m1*CELLS+m2，-1无
};
static TTEntry TT[TT_SIZE];

static void ttClear() { memset(TT, 0, sizeof(TT)); }

static TTEntry* ttProbe(uint64_t key) {
    TTEntry* e = &TT[key & (TT_SIZE-1)];
    return (e->key == key) ? e : nullptr;
}

static void ttStore(uint64_t key, int score, int depth, int flag, int best) {
    TTEntry* e = &TT[key & (TT_SIZE-1)];
    if(e->key == 0 || e->depth <= depth || e->key == key) {
        e->key=key; e->score=score; e->depth=depth; e->flag=flag; e->best=best;
    }
}

// ============================================================
//  Killer + History
// ============================================================
static const int MAX_PLY = 20;
static int killers[MAX_PLY][2]; // encoded move
static int32_t history[2][CELLS][CELLS];

static void clearHeuristics() {
    memset(killers, -1, sizeof(killers));
    memset(history, 0, sizeof(history));
}

static void updateKiller(int ply, int m) {
    if(ply < MAX_PLY) {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = m;
    }
}

static void updateHistory(int color, int m1, int m2, int depth) {
    history[color][m1][m2] += depth*depth;
    history[color][m2][m1] += depth*depth;
}

// ============================================================
//  VCT 搜索（威胁空间搜索）
// ============================================================
// 找 color 的所有"五连威胁"的开放端（对方必须堵的点）
static vector<int> findFiveThreats(const Board& b, int color) {
    vector<int> pts;
    for(int d=0;d<4;d++){
        for(int r=0;r<N;r++){
            for(int c=0;c<N;c++){
                int pr=r-DX[d], pc=c-DY[d];
                if(inBoard(pr,pc)) continue; // 只从线起点
                // 扫描这条线
                int nr=r, nc=c;
                while(inBoard(nr,nc)){
                    if(b.colorAt(rc2i(nr,nc))!=color){ nr+=DX[d]; nc+=DY[d]; continue; }
                    int cnt=0, sr=nr, sc2=nc;
                    while(inBoard(nr,nc)&&b.colorAt(rc2i(nr,nc))==color){ cnt++; nr+=DX[d]; nc+=DY[d]; }
                    if(cnt==5){
                        // 检查两端
                        int er=sr-DX[d], ec=sc2-DY[d];
                        if(inBoard(er,ec)&&b.colorAt(rc2i(er,ec))==EMPTY) pts.push_back(rc2i(er,ec));
                        if(inBoard(nr,nc)&&b.colorAt(rc2i(nr,nc))==EMPTY) pts.push_back(rc2i(nr,nc));
                    }
                }
            }
        }
    }
    // 去重
    sort(pts.begin(),pts.end()); pts.erase(unique(pts.begin(),pts.end()),pts.end());
    return pts;
}

// 检查落两子后是否直接赢（m1==m2 时只落一子）
static bool isWin2(Board& b, int m1, int m2, int color) {
    if(m1 == m2) {
        b.place(m1, color);
        bool w = winsFromPoint(b, m1, color);
        b.remove(m1, color);
        return w;
    }
    b.place(m1,color); b.place(m2,color);
    bool w = winsFromPoint(b,m1,color) || winsFromPoint(b,m2,color);
    b.remove(m2,color); b.remove(m1,color);
    return w;
}

static int chooseFillerMove(const Board& b, int color, int avoid);

static vector<pair<int,int>> collectWinningPairs(const Board& b, int color) {
    vector<pair<int,int>> wins;

    auto addReq = [&](int a, int c) {
        if(a > c) swap(a, c);
        wins.push_back({a, c});
    };

    int opp = 1-color;
    for(int d=0; d<4; d++) {
        for(int r=0; r<N; r++) {
            for(int c=0; c<N; c++) {
                int er = r + DX[d]*5, ec = c + DY[d]*5;
                if(!inBoard(er, ec)) continue;

                int ownCnt = 0, emptyCnt = 0, oppCnt = 0;
                int e0 = -1, e1 = -1;
                for(int k=0; k<6; k++) {
                    int cell = rc2i(r + DX[d]*k, c + DY[d]*k);
                    int v = b.colorAt(cell);
                    if(v == color) ownCnt++;
                    else if(v == opp) oppCnt++;
                    else {
                        if(e0 < 0) e0 = cell;
                        else e1 = cell;
                        emptyCnt++;
                    }
                }

                if(oppCnt != 0) continue;
                if(ownCnt == 5 && emptyCnt == 1) addReq(e0, e0);
                else if(ownCnt == 4 && emptyCnt == 2) addReq(e0, e1);
            }
        }
    }

    sort(wins.begin(), wins.end());
    wins.erase(unique(wins.begin(), wins.end()), wins.end());
    return wins;
}

static bool findFirstWinningPair(Board& b, int color, int& m1, int& m2) {
    auto wins = collectWinningPairs(b, color);
    if(wins.empty()) return false;

    int bestA = -1, bestB = -1;
    int bestScore = -INF;

    for(const auto& w : wins) {
        int a = w.first;
        int c = w.second;
        int b2 = (a == c) ? chooseFillerMove(b, color, a) : c;
        if(a < 0 || b2 < 0 || a == b2) continue;
        if(b.occupied(a) || b.occupied(b2)) continue;

        int sc = quickEval(b, a, color) * 2 + quickEval(b, b2, color);
        if(sc > bestScore) {
            bestScore = sc;
            bestA = a;
            bestB = b2;
        }
    }

    if(bestA < 0 || bestB < 0) return false;
    m1 = bestA;
    m2 = bestB;
    return true;
}

static bool pairHasCell(const pair<int,int>& p, int cell) {
    return p.first == cell || p.second == cell;
}

static int chooseFillerMove(const Board& b, int color, int avoid) {
    auto cands = genCandidates(b, color);
    for(const auto& c : cands) {
        if(c.cell != avoid) return c.cell;
    }
    for(int i=0;i<CELLS;i++) {
        if(i != avoid && !b.occupied(i)) return i;
    }
    return avoid;
}

static bool findDefensePair(Board& b, int threatColor, int myColor, int& m1, int& m2) {
    auto threats = collectWinningPairs(b, threatColor);
    if(threats.empty()) return false;

    vector<int> pool;
    pool.reserve(threats.size() * 2 + 12);
    for(const auto& t : threats) {
        if(!b.occupied(t.first)) pool.push_back(t.first);
        if(!b.occupied(t.second)) pool.push_back(t.second);
    }

    auto cands = genCandidates(b, myColor);
    int extra = min((int)cands.size(), 10);
    for(int i=0; i<extra; i++) {
        int cell = cands[i].cell;
        if(!b.occupied(cell)) pool.push_back(cell);
    }

    sort(pool.begin(), pool.end());
    pool.erase(unique(pool.begin(), pool.end()), pool.end());

    if(pool.size() < 2) {
        int a = threats[0].first;
        int b2 = chooseFillerMove(b, myColor, a);
        if(a >= 0 && b2 >= 0 && a != b2 && !b.occupied(a) && !b.occupied(b2)) {
            m1 = a;
            m2 = b2;
            return true;
        }
        return false;
    }

    int bestA = -1, bestB = -1;
    int bestCover = -1;
    int bestScore = -INF;

    for(int i=0; i<(int)pool.size(); i++) {
        for(int j=i+1; j<(int)pool.size(); j++) {
            int a = pool[i], b2 = pool[j];
            if(b.occupied(a) || b.occupied(b2)) continue;

            int cover = 0;
            for(const auto& t : threats) {
                if(pairHasCell(t, a) || pairHasCell(t, b2)) cover++;
            }

            int sc = quickEval(b, a, myColor) + quickEval(b, b2, myColor)
                   + quickEval(b, a, threatColor) + quickEval(b, b2, threatColor);

            if(cover > bestCover || (cover == bestCover && sc > bestScore)) {
                bestCover = cover;
                bestScore = sc;
                bestA = a;
                bestB = b2;
            }
        }
    }

    if(bestA < 0 || bestB < 0) return false;
    m1 = bestA;
    m2 = bestB;
    return true;
}

// VCT 递归搜索，返回是否必胜，best 存第一步
struct VCTResult { bool win; int m1, m2; };

static VCTResult vctSearch(Board& b, int color, int depth, double timeLimit) {
    if(depth<=0 || elapsed_ms()>=timeLimit) return {false,-1,-1};
    int opp = 1-color;

    // 先找直接赢的两子
    auto cands = genCandidates(b, color);
    int n = min((int)cands.size(), 20);
    for(int i=0;i<n;i++)
        for(int j=i+1;j<n;j++){
            if(isWin2(b, cands[i].cell, cands[j].cell, color))
                return {true, cands[i].cell, cands[j].cell};
        }

    // 找己方五连威胁
    auto threats = findFiveThreats(b, color);
    if(threats.empty()) return {false,-1,-1};

    for(int ti=0; ti<(int)threats.size() && elapsed_ms()<timeLimit; ti++){
        int t = threats[ti];
        for(int ci=0; ci<n && elapsed_ms()<timeLimit; ci++){
            int c2 = cands[ci].cell;
            if(c2 == t) continue;

            b.place(t, color); b.place(c2, color);

            auto newThreats = findFiveThreats(b, color);
            bool doubleThreats = (newThreats.size() >= 2);

            if(doubleThreats) {
                b.remove(c2,color); b.remove(t,color);
                return {true, t, c2};
            }

            if(!newThreats.empty()) {
                // 单威胁：对方只需找到一个有效防守即可阻止我们
                // 正确逻辑：枚举对方所有防守，若存在任意一个防守使我们无法继续VCT，则此威胁无效
                auto oppCands = genCandidates(b, opp);
                int om = min((int)oppCands.size(), 12);
                bool allDefensesLose = true; // 对方所有防守都输 → 我们必胜
                for(int oi=0; oi<om && elapsed_ms()<timeLimit; oi++){
                    for(int oj=oi+1; oj<om && elapsed_ms()<timeLimit; oj++){
                        int od1 = oppCands[oi].cell, od2 = oppCands[oj].cell;
                        b.place(od1, opp); b.place(od2, opp);
                        auto res = vctSearch(b, color, depth-1, timeLimit);
                        b.remove(od2, opp); b.remove(od1, opp);
                        if(!res.win) {
                            // 对方找到了有效防守，此威胁路线失败
                            allDefensesLose = false;
                            goto next_pair; // 跳出双层循环
                        }
                    }
                }
                if(allDefensesLose) {
                    b.remove(c2,color); b.remove(t,color);
                    return {true, t, c2};
                }
                next_pair:;
            }

            b.remove(c2,color); b.remove(t,color);
        }
    }
    return {false,-1,-1};
}

// ============================================================
//  Negamax Alpha-Beta
// ============================================================
static Move2 g_bestRoot;

static int negamax(Board& b, int depth, int alpha, int beta, int color, int ply) {
    // 终局检测：上一步（1-color）是否已经赢了
    // 注意：必须在 timeUp 之前检测，否则超时时漏掉必输局面
    if(checkWin(b, 1-color)) return LOSE_SCORE + ply;

    if(timeUp()) return evalBoard(b, color);
    if(depth == 0) return evalBoard(b, color);

    uint64_t key = b.zobrist ^ (color==WHITE ? ZOBRIST_TURN : 0);

    // TT 查询
    TTEntry* tte = ttProbe(key);
    int ttBest = -1;
    if(tte && tte->depth >= depth) {
        int s = tte->score;
        if(tte->flag == 0) return s;
        if(tte->flag == 1 && s >= beta)  return s;
        if(tte->flag == 2 && s <= alpha) return s;
        ttBest = tte->best;
    } else if(tte) {
        ttBest = tte->best;
    }

    int candN = (depth >= 3) ? 10 : 14;
    auto moves = genMovePairs(b, color, candN);
    if(moves.empty()) return evalBoard(b, color);

    // 走法排序：TT走法提前
    if(ttBest >= 0) {
        for(auto& m : moves)
            if(m.m1*CELLS+m.m2 == ttBest || m.m2*CELLS+m.m1 == ttBest)
                { m.score += 10000000; break; }
        sort(moves.begin(), moves.end(), [](const Move2& a, const Move2& b){ return a.score > b.score; });
    }
    // Killer 提前
    if(ply < MAX_PLY) {
        for(int k=0;k<2;k++){
            int km = killers[ply][k];
            if(km < 0) continue;
            for(auto& m : moves)
                if(m.m1*CELLS+m.m2==km || m.m2*CELLS+m.m1==km)
                    { m.score += 1000000; break; }
        }
        sort(moves.begin(), moves.end(), [](const Move2& a, const Move2& b){ return a.score > b.score; });
    }
    // History
    for(auto& m : moves)
        m.score += history[color][m.m1][m.m2];

    int origAlpha = alpha;
    int bestScore = -INF;
    int bestEnc = -1;

    for(auto& m : moves) {
        if(timeUp()) break;
        b.place(m.m1, color); b.place(m.m2, color);
        int sc = -negamax(b, depth-1, -beta, -alpha, 1-color, ply+1);
        b.remove(m.m2, color); b.remove(m.m1, color);

        if(sc > bestScore) {
            bestScore = sc;
            bestEnc = m.m1*CELLS+m.m2;
            if(ply == 0) g_bestRoot = {m.m1, m.m2, sc};
        }
        if(sc > alpha) alpha = sc;
        if(alpha >= beta) {
            updateKiller(ply, bestEnc);
            updateHistory(color, m.m1, m.m2, depth);
            break;
        }
    }

    int flag = (bestScore <= origAlpha) ? 2 : (bestScore >= beta ? 1 : 0);
    ttStore(key, bestScore, depth, flag, bestEnc);
    return bestScore;
}

// ============================================================
//  迭代加深
// ============================================================
static Move2 iterDeep(Board& b, int color) {
    clearHeuristics();
    g_bestRoot = {-1,-1,0};

    // 初始最优（防止超时没有结果）
    auto cands = genCandidates(b, color);
    if(cands.size() >= 2)
        g_bestRoot = {cands[0].cell, cands[1].cell, 0};
    else if(cands.size() == 1) {
        // 找一个不同的邻居格子作为第二子
        int c0 = cands[0].cell;
        int r0 = i2r(c0), col0 = i2c(c0);
        int c1 = -1;
        for(int dr=-1; dr<=1 && c1<0; dr++)
            for(int dc=-1; dc<=1 && c1<0; dc++){
                if(dr==0&&dc==0) continue;
                int nr=r0+dr, nc=col0+dc;
                if(inBoard(nr,nc) && !b.occupied(rc2i(nr,nc)))
                    c1 = rc2i(nr,nc);
            }
        if(c1 < 0) c1 = c0; // 极端情况兜底
        g_bestRoot = {c0, c1, 0};
    }

    int prevScore = 0;
    for(int depth=1; depth<=9 && !timeUp(); depth++) {
        int alpha = -INF, beta = INF;
        // Aspiration window
        if(depth >= 3) { alpha = prevScore-3000; beta = prevScore+3000; }

        int sc = negamax(b, depth, alpha, beta, color, 0);

        if((sc <= alpha || sc >= beta) && !timeUp())
            sc = negamax(b, depth, -INF, INF, color, 0);

        if(!timeUp()) prevScore = sc;
        if(sc >= WIN_SCORE-1000 || sc <= LOSE_SCORE+1000) break;
    }
    return g_bestRoot;
}

// ============================================================
//  简易 JSON 解析（不依赖外部库）
// ============================================================
// 在 JSON 对象字符串中找 "key": <int>，返回该整数，找不到返回 INT_MIN
static int jsonGetInt(const string& s, const string& key, size_t from=0) {
    // 精确匹配 "key"，避免 "x" 匹配到 "x2"
    string pat = "\"" + key + "\"";
    size_t p = from;
    while(true) {
        p = s.find(pat, p);
        if(p == string::npos) return INT_MIN;
        // 确认后面紧跟的是 ':' 或空白+':'，不是其他字符（如 "x2" 中的 '2'）
        size_t q = p + pat.size();
        while(q < s.size() && (s[q]==' '||s[q]=='\t')) q++;
        if(q < s.size() && s[q] == ':') break;
        p += pat.size(); // 继续找下一个
    }
    p = s.find(':', p + pat.size());
    if(p == string::npos) return INT_MIN;
    p++;
    while(p < s.size() && (s[p]==' '||s[p]=='\t'||s[p]=='\n'||s[p]=='\r')) p++;
    if(p >= s.size()) return INT_MIN;
    int sign = 1;
    if(s[p]=='-') { sign=-1; p++; }
    if(p >= s.size() || !isdigit((unsigned char)s[p])) return INT_MIN;
    int val = 0;
    while(p < s.size() && isdigit((unsigned char)s[p])) { val=val*10+(s[p]-'0'); p++; }
    return sign*val;
}

struct Move4 { int x,y,x2,y2; };

// 提取 JSON 数组 arrKey 中每个对象的 x0/y0/x1/y1
static vector<Move4> jsonGetArray(const string& s, const string& arrKey) {
    vector<Move4> res;
    string pat = "\"" + arrKey + "\"";
    size_t p = s.find(pat);
    if(p == string::npos) return res;
    p = s.find('[', p + pat.size());
    if(p == string::npos) return res;
    int dep = 0;
    size_t start = string::npos;
    for(size_t i = p+1; i < s.size(); i++) {
        if(s[i]=='{') {
            if(dep==0) start=i;
            dep++;
        } else if(s[i]=='}') {
            dep--;
            if(dep==0 && start!=string::npos) {
                string obj = s.substr(start, i-start+1);
                Move4 m;
                int vx  = jsonGetInt(obj,"x0");
                int vy  = jsonGetInt(obj,"y0");
                int vx2 = jsonGetInt(obj,"x1");
                int vy2 = jsonGetInt(obj,"y1");
                m.x  = (vx  == INT_MIN) ? -1 : vx;
                m.y  = (vy  == INT_MIN) ? -1 : vy;
                m.x2 = (vx2 == INT_MIN) ? -1 : vx2;
                m.y2 = (vy2 == INT_MIN) ? -1 : vy2;
                res.push_back(m);
                start = string::npos;
            }
        } else if(s[i]==']' && dep==0) break;
    }
    return res;
}

// ============================================================
//  主函数：Botzone JSON 交互模式
// ============================================================
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    g_start = chrono::steady_clock::now();
    initZobrist();
    initNeighbor2();
    ttClear();

    // 读入完整 JSON
    string input, line;
    while(getline(cin, line)) { input += line; input += '\n'; }

    auto requests  = jsonGetArray(input, "requests");
    auto responses = jsonGetArray(input, "responses");

    Board board;
    // requests[0].x == -1 → 我方先手黑棋
    int myColor = (requests.empty() || requests[0].x < 0) ? BLACK : WHITE;

    // 重放历史：requests[i] 是对方走法，responses[i] 是己方走法
    for(int i = 0; i < (int)requests.size(); i++) {
        int opp = 1 - myColor;
        auto& req = requests[i];
        if(req.x >= 0 && req.x < N && req.y >= 0 && req.y < N) {
            board.place(rc2i(req.x, req.y), opp);
            if(req.x2 >= 0 && req.x2 < N && req.y2 >= 0 && req.y2 < N)
                board.place(rc2i(req.x2, req.y2), opp);
        }
        if(i < (int)responses.size()) {
            auto& rsp = responses[i];
            if(rsp.x >= 0 && rsp.x < N && rsp.y >= 0 && rsp.y < N) {
                board.place(rc2i(rsp.x, rsp.y), myColor);
                if(rsp.x2 >= 0 && rsp.x2 < N && rsp.y2 >= 0 && rsp.y2 < N)
                    board.place(rc2i(rsp.x2, rsp.y2), myColor);
            }
        }
    }

    int outX0=9, outY0=9, outX1=-1, outY1=-1;

    bool firstMove = (requests.size() == 1 && requests[0].x < 0);

    if(firstMove) {
        // 黑棋第一手天元，只落一子
        outX0=9; outY0=9; outX1=-1; outY1=-1;
    } else {
        int m1=-1, m2=-1;
        int opp = 1-myColor;

        if(findFirstWinningPair(board, myColor, m1, m2)) {
            outX0=i2r(m1); outY0=i2c(m1);
            outX1=i2r(m2); outY1=i2c(m2);
        } else if(findDefensePair(board, opp, myColor, m1, m2)) {
            outX0=i2r(m1); outY0=i2c(m1);
            outX1=i2r(m2); outY1=i2c(m2);
        } else {
        // 1. VCT 必胜搜索（120ms 预算）
        double vctLimit = elapsed_ms() + 120.0;
        auto vct = vctSearch(board, myColor, 8, vctLimit);
        if(vct.win && vct.m1 >= 0 && vct.m2 >= 0) {
            outX0=i2r(vct.m1); outY0=i2c(vct.m1);
            outX1=i2r(vct.m2); outY1=i2c(vct.m2);
        } else {
            // 2. 检查对方是否立即能赢（必须堵）
            int opp = 1-myColor;
            auto oppC = genCandidates(board, opp);
            int om = min((int)oppC.size(), 15);
            bool blocked = false;
            for(int i=0;i<om&&!blocked;i++)
                for(int j=i+1;j<om&&!blocked;j++)
                    if(isWin2(board, oppC[i].cell, oppC[j].cell, opp)){
                        outX0=i2r(oppC[i].cell); outY0=i2c(oppC[i].cell);
                        outX1=i2r(oppC[j].cell); outY1=i2c(oppC[j].cell);
                        blocked=true;
                    }
            if(!blocked){
                // 3. 迭代加深 Alpha-Beta
                auto best = iterDeep(board, myColor);
                if(best.m1 >= 0 && best.m2 >= 0) {
                    outX0=i2r(best.m1); outY0=i2c(best.m1);
                    outX1=i2r(best.m2); outY1=i2c(best.m2);
                }
            }
        }
    }

    // Botzone JSON 输出格式
        }

    cout << "{\"response\":{\"x0\":" << outX0
         << ",\"y0\":" << outY0
         << ",\"x1\":" << outX1
         << ",\"y1\":" << outY1
         << "}}" << endl;
    return 0;
}
