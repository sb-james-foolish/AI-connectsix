/**
 * 用于 Botzone 平台的六子棋 AI 代码
 * 核心实现：
 * 1) 初始局面仅执行一次全局 924 路扫描；
 * 2) 博弈树扩展仅执行局部“路”增量扫描；
 * 3) 估值体系严格使用固定权重表。
 */
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _BOTZONE_ONLINE
#include "jsoncpp/json.h"
#else
#include <json/json.h>
#endif

using Score = long long;

struct Move
{
    int x = -1;
    int y = -1;

    Move() = default;
    Move(int x_, int y_) : x(x_), y(y_) {}
};

struct MovePair
{
    Move first{};
    Move second{};
    bool singleStoneTurn = false;
};

class Game
{
public:
    enum class Player
    {
        kNone = 0,
        kBlack = 1,
        kWhite = 2
    };

    static constexpr int kBoardWidth = 15;
    static constexpr int kBoardHeight = 15;
    static constexpr int kWinLength = 6;
    static constexpr int kTotalRoads = 500;
    static constexpr Score kMateScore = 1000000000000LL;

public:
    Game()
    {
        for (auto &column : board_)
        {
            column.fill(Player::kNone);
        }
        initRoads();
    }

    void setMyPlayer(Player player) { myPlayer_ = player; }
    Player getMyPlayer() const { return myPlayer_; }
    Player getOppPlayer() const { return toOpp(myPlayer_); }

    Player toOpp(Player player) const
    {
        if (player == Player::kBlack)
        {
            return Player::kWhite;
        }
        if (player == Player::kWhite)
        {
            return Player::kBlack;
        }
        return Player::kNone;
    }

    bool isInBoard(int x, int y) const
    {
        return x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight;
    }

    Player getPlayerAt(int x, int y) const
    {
        if (!isInBoard(x, y))
        {
            return Player::kNone;
        }
        return board_[x][y];
    }

    bool makeMove(int x, int y, Player player)
    {
        if (!isInBoard(x, y) || player == Player::kNone || board_[x][y] != Player::kNone)
        {
            return false;
        }
        board_[x][y] = player;
        return true;
    }

    bool makeMove(const Move &move, Player player)
    {
        return makeMove(move.x, move.y, player);
    }

    void initializeGlobalEvaluation()
    {
        // 仅在局面初始化时进行一次全局 924 路扫描。
        totalScore_ = 0;
        for (size_t i = 0; i < roads_.size(); ++i)
        {
            roadStates_[i] = computeRoadState(static_cast<int>(i));
            totalScore_ += roadStates_[i].score;
        }
    }

    MovePair searchBestMovePair(int depth, int width, bool singleStoneTurn)
    {
        const int safeDepth = std::max(1, depth);
        const int safeWidth = std::max(1, width);
        searchWidth_ = safeWidth;

        MovePair fallback = buildFallbackMovePair(singleStoneTurn);
        if (isBoardFull())
        {
            return fallback;
        }

        std::vector<Candidate> rootCandidates = generateCandidates(
            myPlayer_,
            singleStoneTurn,
            std::max(safeWidth * 4, safeWidth),
            true);

        if (rootCandidates.empty())
        {
            return fallback;
        }

        // 优先级1：存在可直接成 6 连的落子，必须优先返回。
        Candidate *bestImmediate = nullptr;
        for (auto &candidate : rootCandidates)
        {
            if (!candidate.immediateWin)
            {
                continue;
            }
            if (bestImmediate == nullptr || candidate.heuristic > bestImmediate->heuristic)
            {
                bestImmediate = &candidate;
            }
        }
        if (bestImmediate != nullptr)
        {
            return bestImmediate->movePair;
        }

        // 宽度裁剪：保留前 Width 个落子组合进行递归扩展。
        if (static_cast<int>(rootCandidates.size()) > safeWidth)
        {
            rootCandidates.resize(static_cast<size_t>(safeWidth));
        }

        const Score kInf = std::numeric_limits<Score>::max() / 4;
        Score alpha = -kInf;
        Score beta = kInf;
        Score bestScore = -kInf;
        MovePair bestMove = rootCandidates.front().movePair;

        for (const Candidate &candidate : rootCandidates)
        {
            std::vector<Move> moves = unpackMovePair(candidate.movePair);
            EvalUndo undo = applyMovesWithLocalScan(moves, myPlayer_);

            Score score = 0;
            if (hasConnectSixInAffectedRoads(myPlayer_, undo.affectedRoads))
            {
                score = kMateScore + safeDepth;
            }
            else
            {
                score = alphaBeta(safeDepth - 1, toOpp(myPlayer_), alpha, beta);
            }

            undoMoves(undo);

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = candidate.movePair;
            }
            alpha = std::max(alpha, bestScore);
        }

        return bestMove;
    }

private:
    struct Road
    {
        std::array<Move, kWinLength> points{};
    };

    struct RoadState
    {
        int blackCount = 0;
        int whiteCount = 0;
        Score score = 0;
    };

    struct EvalUndo
    {
        Score previousTotal = 0;
        std::vector<int> affectedRoads;
        std::vector<RoadState> previousRoadStates;
        std::vector<Move> placedMoves;
    };

    struct Candidate
    {
        MovePair movePair{};
        Score heuristic = 0;
        bool immediateWin = false;
        bool blockThreat = false;
    };

private:
    static constexpr std::array<Score, 7> kAttackWeights{
        0, 1, 20, 40, 200, 200, 1000000};
    static constexpr std::array<Score, 7> kDefenseWeights{
        0, 1, 25, 50, 6000, 6000, 1000000};

private:
    int toIndex(int x, int y) const
    {
        return x * kBoardHeight + y;
    }

    bool isLegalMove(const Move &move) const
    {
        return isInBoard(move.x, move.y) && board_[move.x][move.y] == Player::kNone;
    }

    bool isValidCoordinate(const Move &move) const
    {
        return move.x >= 0 && move.y >= 0 && isInBoard(move.x, move.y);
    }

    Score composeRoadScore(int blackCount, int whiteCount) const
    {
        // 混色路视为无效路，不参与估值。
        if (blackCount > 0 && whiteCount > 0)
        {
            return 0;
        }
        if (blackCount == 0 && whiteCount == 0)
        {
            return 0;
        }

        const int myCount = (myPlayer_ == Player::kBlack) ? blackCount : whiteCount;
        const int oppCount = (myPlayer_ == Player::kBlack) ? whiteCount : blackCount;

        if (myCount > 0)
        {
            return kAttackWeights[myCount];
        }
        return -kDefenseWeights[oppCount];
    }

    RoadState computeRoadState(int roadIndex) const
    {
        RoadState state{};
        const Road &road = roads_[roadIndex];

        for (const Move &point : road.points)
        {
            const Player p = board_[point.x][point.y];
            if (p == Player::kBlack)
            {
                ++state.blackCount;
            }
            else if (p == Player::kWhite)
            {
                ++state.whiteCount;
            }
        }
        state.score = composeRoadScore(state.blackCount, state.whiteCount);
        return state;
    }

    void initRoads()
    {
        roads_.clear();
        roads_.reserve(kTotalRoads);

        // 横向路
        for (int y = 0; y < kBoardHeight; ++y)
        {
            for (int x = 0; x <= kBoardWidth - kWinLength; ++x)
            {
                Road road{};
                for (int k = 0; k < kWinLength; ++k)
                {
                    road.points[k] = Move(x + k, y);
                }
                roads_.push_back(road);
            }
        }

        // 纵向路
        for (int x = 0; x < kBoardWidth; ++x)
        {
            for (int y = 0; y <= kBoardHeight - kWinLength; ++y)
            {
                Road road{};
                for (int k = 0; k < kWinLength; ++k)
                {
                    road.points[k] = Move(x, y + k);
                }
                roads_.push_back(road);
            }
        }

        // 右下斜向路
        for (int x = 0; x <= kBoardWidth - kWinLength; ++x)
        {
            for (int y = 0; y <= kBoardHeight - kWinLength; ++y)
            {
                Road road{};
                for (int k = 0; k < kWinLength; ++k)
                {
                    road.points[k] = Move(x + k, y + k);
                }
                roads_.push_back(road);
            }
        }

        // 右上斜向路
        for (int x = 0; x <= kBoardWidth - kWinLength; ++x)
        {
            for (int y = kWinLength - 1; y < kBoardHeight; ++y)
            {
                Road road{};
                for (int k = 0; k < kWinLength; ++k)
                {
                    road.points[k] = Move(x + k, y - k);
                }
                roads_.push_back(road);
            }
        }

        roadStates_.assign(roads_.size(), RoadState{});

        for (auto &refs : pointToRoads_)
        {
            refs.clear();
        }
        for (size_t roadIndex = 0; roadIndex < roads_.size(); ++roadIndex)
        {
            for (const Move &point : roads_[roadIndex].points)
            {
                pointToRoads_[toIndex(point.x, point.y)].push_back(static_cast<int>(roadIndex));
            }
        }
    }

    bool hasConnectSix(Player player) const
    {
        for (const RoadState &state : roadStates_)
        {
            if (player == Player::kBlack && state.blackCount == kWinLength && state.whiteCount == 0)
            {
                return true;
            }
            if (player == Player::kWhite && state.whiteCount == kWinLength && state.blackCount == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool hasConnectSixInAffectedRoads(Player player, const std::vector<int> &affectedRoads) const
    {
        for (int roadIndex : affectedRoads)
        {
            const RoadState &state = roadStates_[roadIndex];
            if (player == Player::kBlack && state.blackCount == kWinLength && state.whiteCount == 0)
            {
                return true;
            }
            if (player == Player::kWhite && state.whiteCount == kWinLength && state.blackCount == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool isBoardFull() const
    {
        for (int x = 0; x < kBoardWidth; ++x)
        {
            for (int y = 0; y < kBoardHeight; ++y)
            {
                if (board_[x][y] == Player::kNone)
                {
                    return false;
                }
            }
        }
        return true;
    }

    std::vector<Move> unpackMovePair(const MovePair &movePair) const
    {
        std::vector<Move> moves;
        if (isValidCoordinate(movePair.first))
        {
            moves.push_back(movePair.first);
        }
        if (!movePair.singleStoneTurn && isValidCoordinate(movePair.second))
        {
            moves.push_back(movePair.second);
        }
        return moves;
    }

    EvalUndo applyMovesWithLocalScan(const std::vector<Move> &moves, Player player)
    {
        EvalUndo undo{};
        undo.previousTotal = totalScore_;
        undo.affectedRoads.reserve(48);
        undo.previousRoadStates.reserve(48);
        undo.placedMoves.reserve(moves.size());

        if (player == Player::kNone || moves.empty())
        {
            return undo;
        }

        // 只收集受新增落子影响的局部路。
        std::array<char, kTotalRoads> touched{};
        for (const Move &move : moves)
        {
            if (!isLegalMove(move))
            {
                continue;
            }
            for (int roadIndex : pointToRoads_[toIndex(move.x, move.y)])
            {
                if (!touched[roadIndex])
                {
                    touched[roadIndex] = 1;
                    undo.affectedRoads.push_back(roadIndex);
                }
            }
        }

        Score oldLocalScore = 0;
        for (int roadIndex : undo.affectedRoads)
        {
            undo.previousRoadStates.push_back(roadStates_[roadIndex]);
            oldLocalScore += roadStates_[roadIndex].score;
        }

        for (const Move &move : moves)
        {
            if (!isLegalMove(move))
            {
                continue;
            }
            board_[move.x][move.y] = player;
            undo.placedMoves.push_back(move);
        }

        Score newLocalScore = 0;
        for (int roadIndex : undo.affectedRoads)
        {
            roadStates_[roadIndex] = computeRoadState(roadIndex);
            newLocalScore += roadStates_[roadIndex].score;
        }

        if (!undo.affectedRoads.empty())
        {
            totalScore_ = totalScore_ - oldLocalScore + newLocalScore;
        }

        return undo;
    }

    void undoMoves(const EvalUndo &undo)
    {
        for (auto it = undo.placedMoves.rbegin(); it != undo.placedMoves.rend(); ++it)
        {
            board_[it->x][it->y] = Player::kNone;
        }

        for (size_t i = 0; i < undo.affectedRoads.size(); ++i)
        {
            roadStates_[undo.affectedRoads[i]] = undo.previousRoadStates[i];
        }

        totalScore_ = undo.previousTotal;
    }

    std::vector<Move> collectThreatPoints(Player attacker, int minCount) const
    {
        std::vector<Move> threatPoints;
        std::array<std::array<bool, kBoardHeight>, kBoardWidth> used{};
        for (auto &column : used)
        {
            column.fill(false);
        }

        for (size_t i = 0; i < roads_.size(); ++i)
        {
            const RoadState &state = roadStates_[i];
            const int attackCount = (attacker == Player::kBlack) ? state.blackCount : state.whiteCount;
            const int defendCount = (attacker == Player::kBlack) ? state.whiteCount : state.blackCount;

            if (attackCount >= minCount && attackCount < kWinLength && defendCount == 0)
            {
                for (const Move &point : roads_[i].points)
                {
                    if (board_[point.x][point.y] != Player::kNone)
                    {
                        continue;
                    }
                    if (!used[point.x][point.y])
                    {
                        used[point.x][point.y] = true;
                        threatPoints.push_back(point);
                    }
                }
            }
        }
        return threatPoints;
    }

    std::vector<Move> generateAllLegalPoints() const
    {
        std::vector<Move> points;
        points.reserve(kBoardWidth * kBoardHeight);

        for (int x = 0; x < kBoardWidth; ++x)
        {
            for (int y = 0; y < kBoardHeight; ++y)
            {
                if (board_[x][y] == Player::kNone)
                {
                    points.emplace_back(x, y);
                }
            }
        }
        return points;
    }

    std::vector<Move> generateFrontierPoints(int radius) const
    {
        std::vector<Move> points;
        std::array<std::array<bool, kBoardHeight>, kBoardWidth> selected{};
        for (auto &column : selected)
        {
            column.fill(false);
        }

        bool hasAnyStone = false;
        for (int x = 0; x < kBoardWidth; ++x)
        {
            for (int y = 0; y < kBoardHeight; ++y)
            {
                if (board_[x][y] == Player::kNone)
                {
                    continue;
                }

                hasAnyStone = true;
                for (int dx = -radius; dx <= radius; ++dx)
                {
                    for (int dy = -radius; dy <= radius; ++dy)
                    {
                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (!isInBoard(nx, ny) || board_[nx][ny] != Player::kNone || selected[nx][ny])
                        {
                            continue;
                        }
                        selected[nx][ny] = true;
                        points.emplace_back(nx, ny);
                    }
                }
            }
        }

        if (!hasAnyStone)
        {
            points.emplace_back(kBoardWidth / 2, kBoardHeight / 2);
            return points;
        }

        if (points.empty())
        {
            return generateAllLegalPoints();
        }
        return points;
    }

    Score absScore(Score value) const
    {
        return value >= 0 ? value : -value;
    }

    Score evaluatePointPriority(Player player, const Move &point, bool isThreatPoint)
    {
        if (!isLegalMove(point))
        {
            return std::numeric_limits<Score>::lowest() / 4;
        }

        const Score base = totalScore_;
        EvalUndo undo = applyMovesWithLocalScan(std::vector<Move>{point}, player);

        const Score delta = totalScore_ - base;
        Score priority = (player == myPlayer_) ? delta : -delta;
        priority += absScore(delta) / 2;

        if (hasConnectSixInAffectedRoads(player, undo.affectedRoads))
        {
            priority += kMateScore / 16;
        }
        if (isThreatPoint)
        {
            priority += 20000000;
        }

        undoMoves(undo);
        return priority;
    }

    std::vector<Move> pickExpansionPoints(
        Player player,
        bool singleStoneTurn,
        int width,
        const std::vector<Move> &threatPoints,
        const std::array<std::array<bool, kBoardHeight>, kBoardWidth> &threatMask)
    {
        struct ScoredPoint
        {
            Move point{};
            Score priority = 0;
        };

        std::vector<Move> frontierPoints = generateFrontierPoints(2);
        if (frontierPoints.empty())
        {
            return frontierPoints;
        }

        const int pointCap = singleStoneTurn ? 30 : 24;
        int targetPointCount = singleStoneTurn ? (width + 4) : (width + 2);
        targetPointCount = std::max(targetPointCount, singleStoneTurn ? 12 : 10);
        targetPointCount = std::min(targetPointCount, pointCap);

        std::array<std::array<bool, kBoardHeight>, kBoardWidth> selected{};
        for (auto &column : selected)
        {
            column.fill(false);
        }

        std::vector<Move> selectedPoints;
        selectedPoints.reserve(static_cast<size_t>(targetPointCount));

        auto tryAdd = [&](const Move &point)
        {
            if (!isLegalMove(point) || selected[point.x][point.y])
            {
                return false;
            }
            selected[point.x][point.y] = true;
            selectedPoints.push_back(point);
            return true;
        };

        if (!threatPoints.empty())
        {
            std::vector<ScoredPoint> threatScored;
            threatScored.reserve(threatPoints.size());
            for (const Move &point : threatPoints)
            {
                if (!isLegalMove(point))
                {
                    continue;
                }
                threatScored.push_back(ScoredPoint{point, evaluatePointPriority(player, point, true)});
            }

            std::sort(threatScored.begin(), threatScored.end(), [](const ScoredPoint &lhs, const ScoredPoint &rhs)
                      { return lhs.priority > rhs.priority; });

            int threatLimit = std::max(5, width);
            threatLimit = std::min(threatLimit, pointCap);
            threatLimit = std::min(threatLimit, static_cast<int>(threatScored.size()));

            for (int i = 0; i < threatLimit; ++i)
            {
                if (static_cast<int>(selectedPoints.size()) >= targetPointCount)
                {
                    break;
                }
                tryAdd(threatScored[static_cast<size_t>(i)].point);
            }
        }

        std::vector<ScoredPoint> scored;
        scored.reserve(frontierPoints.size());
        for (const Move &point : frontierPoints)
        {
            if (selected[point.x][point.y])
            {
                continue;
            }
            const bool isThreatPoint = threatMask[point.x][point.y];
            scored.push_back(ScoredPoint{point, evaluatePointPriority(player, point, isThreatPoint)});
        }

        std::sort(scored.begin(), scored.end(), [](const ScoredPoint &lhs, const ScoredPoint &rhs)
                  { return lhs.priority > rhs.priority; });

        for (const ScoredPoint &item : scored)
        {
            if (static_cast<int>(selectedPoints.size()) >= targetPointCount)
            {
                break;
            }
            tryAdd(item.point);
        }

        if (selectedPoints.empty() && !frontierPoints.empty())
        {
            selectedPoints.push_back(frontierPoints.front());
        }

        return selectedPoints;
    }

    std::vector<Candidate> generateCandidates(Player player, bool singleStoneTurn, int width, bool applyDefensePriority)
    {
        std::vector<Candidate> candidates;

        std::array<std::array<bool, kBoardHeight>, kBoardWidth> threatMask{};
        for (auto &column : threatMask)
        {
            column.fill(false);
        }

        std::vector<Move> threatPoints = collectThreatPoints(toOpp(player), 4);
        for (const Move &point : threatPoints)
        {
            threatMask[point.x][point.y] = true;
        }

        std::vector<Move> topPoints = pickExpansionPoints(player, singleStoneTurn, width, threatPoints, threatMask);
        if (topPoints.empty())
        {
            return candidates;
        }

        auto buildCandidate = [&](const MovePair &movePair)
        {
            Candidate candidate{};
            candidate.movePair = movePair;

            bool blockThreat = false;
            if (isValidCoordinate(movePair.first) && threatMask[movePair.first.x][movePair.first.y])
            {
                blockThreat = true;
            }
            if (!movePair.singleStoneTurn && isValidCoordinate(movePair.second) &&
                threatMask[movePair.second.x][movePair.second.y])
            {
                blockThreat = true;
            }
            candidate.blockThreat = blockThreat;

            std::vector<Move> moves = unpackMovePair(movePair);
            EvalUndo undo = applyMovesWithLocalScan(moves, player);
            candidate.immediateWin = hasConnectSixInAffectedRoads(player, undo.affectedRoads);
            candidate.heuristic = totalScore_;

            if (candidate.immediateWin)
            {
                candidate.heuristic = (player == myPlayer_) ? kMateScore : -kMateScore;
            }

            undoMoves(undo);
            if (moves.empty())
            {
                return;
            }
            candidates.push_back(candidate);
        };

        if (singleStoneTurn)
        {
            for (const Move &p0 : topPoints)
            {
                MovePair pair{};
                pair.first = p0;
                pair.second = Move(-1, -1);
                pair.singleStoneTurn = true;
                buildCandidate(pair);
            }
        }
        else
        {
            for (size_t i = 0; i < topPoints.size(); ++i)
            {
                for (size_t j = i + 1; j < topPoints.size(); ++j)
                {
                    MovePair pair{};
                    pair.first = topPoints[i];
                    pair.second = topPoints[j];
                    pair.singleStoneTurn = false;
                    buildCandidate(pair);
                }
            }
        }

        bool hasImmediateWin = false;
        for (const Candidate &candidate : candidates)
        {
            if (candidate.immediateWin)
            {
                hasImmediateWin = true;
                break;
            }
        }

        // 优先级2：对手存在 4/5 威胁路时，且本方无直接胜利，优先保留防守点组合。
        if (applyDefensePriority && !hasImmediateWin && !threatPoints.empty())
        {
            std::vector<Candidate> defenseCandidates;
            defenseCandidates.reserve(candidates.size());
            for (const Candidate &candidate : candidates)
            {
                if (candidate.blockThreat)
                {
                    defenseCandidates.push_back(candidate);
                }
            }
            if (!defenseCandidates.empty())
            {
                candidates.swap(defenseCandidates);
            }
        }

        std::sort(candidates.begin(), candidates.end(), [&](const Candidate &lhs, const Candidate &rhs)
                  {
            if (lhs.immediateWin != rhs.immediateWin)
            {
                return lhs.immediateWin;
            }
            if (player == myPlayer_)
            {
                if (lhs.heuristic != rhs.heuristic)
                {
                    return lhs.heuristic > rhs.heuristic;
                }
            }
            else
            {
                if (lhs.heuristic != rhs.heuristic)
                {
                    return lhs.heuristic < rhs.heuristic;
                }
            }
            return lhs.blockThreat && !rhs.blockThreat; });

        if (static_cast<int>(candidates.size()) > width)
        {
            candidates.resize(static_cast<size_t>(width));
        }

        return candidates;
    }

    Score alphaBeta(int depth, Player player, Score alpha, Score beta)
    {
        if (hasConnectSix(myPlayer_))
        {
            return kMateScore + depth;
        }
        if (hasConnectSix(getOppPlayer()))
        {
            return -kMateScore - depth;
        }
        if (depth <= 0 || isBoardFull())
        {
            return totalScore_;
        }

        std::vector<Candidate> candidates = generateCandidates(player, false, searchWidth_, true);
        if (candidates.empty())
        {
            return totalScore_;
        }

        const Score kInf = std::numeric_limits<Score>::max() / 4;

        if (player == myPlayer_)
        {
            Score best = -kInf;
            for (const Candidate &candidate : candidates)
            {
                std::vector<Move> moves = unpackMovePair(candidate.movePair);
                EvalUndo undo = applyMovesWithLocalScan(moves, player);

                Score score = 0;
                if (hasConnectSixInAffectedRoads(player, undo.affectedRoads))
                {
                    score = kMateScore + depth;
                }
                else
                {
                    score = alphaBeta(depth - 1, toOpp(player), alpha, beta);
                }

                undoMoves(undo);

                best = std::max(best, score);
                alpha = std::max(alpha, best);
                if (alpha >= beta)
                {
                    break;
                }
            }
            return best;
        }

        Score best = kInf;
        for (const Candidate &candidate : candidates)
        {
            std::vector<Move> moves = unpackMovePair(candidate.movePair);
            EvalUndo undo = applyMovesWithLocalScan(moves, player);

            Score score = 0;
            if (hasConnectSixInAffectedRoads(player, undo.affectedRoads))
            {
                score = -kMateScore - depth;
            }
            else
            {
                score = alphaBeta(depth - 1, toOpp(player), alpha, beta);
            }

            undoMoves(undo);

            best = std::min(best, score);
            beta = std::min(beta, best);
            if (alpha >= beta)
            {
                break;
            }
        }
        return best;
    }

    MovePair buildFallbackMovePair(bool singleStoneTurn) const
    {
        Move first(-1, -1);
        Move second(-1, -1);

        for (int x = 0; x < kBoardWidth; ++x)
        {
            for (int y = 0; y < kBoardHeight; ++y)
            {
                if (board_[x][y] != Player::kNone)
                {
                    continue;
                }
                if (!isValidCoordinate(first))
                {
                    first = Move(x, y);
                }
                else if (!isValidCoordinate(second))
                {
                    second = Move(x, y);
                    break;
                }
            }
            if (isValidCoordinate(second))
            {
                break;
            }
        }

        if (!isValidCoordinate(first))
        {
            return MovePair{Move(-1, -1), Move(-1, -1), true};
        }

        if (singleStoneTurn || !isValidCoordinate(second))
        {
            return MovePair{first, Move(-1, -1), true};
        }

        return MovePair{first, second, false};
    }

private:
    Player myPlayer_ = Player::kBlack;
    Score totalScore_ = 0;
    int searchWidth_ = 10;

    std::array<std::array<Player, kBoardHeight>, kBoardWidth> board_{};
    std::vector<Road> roads_;
    std::vector<RoadState> roadStates_;
    std::array<std::vector<int>, kBoardWidth * kBoardHeight> pointToRoads_{};
};

int readOptionInt(const Json::Value &inputJson, const char *key, int defaultValue)
{
    if (inputJson.isMember(key) && inputJson[key].isInt())
    {
        return inputJson[key].asInt();
    }

    if (inputJson.isMember("data") && inputJson["data"].isObject() &&
        inputJson["data"].isMember(key) && inputJson["data"][key].isInt())
    {
        return inputJson["data"][key].asInt();
    }

    return defaultValue;
}

void applyJsonMovePair(Game &game, const Json::Value &node, Game::Player player)
{
    if (!node.isObject())
    {
        return;
    }

    const int x0 = node.get("x0", -1).asInt();
    const int y0 = node.get("y0", -1).asInt();
    const int x1 = node.get("x1", -1).asInt();
    const int y1 = node.get("y1", -1).asInt();

    game.makeMove(x0, y0, player);
    game.makeMove(x1, y1, player);
}

int main()
{
    std::string inputStr;
    std::string line;
    while (std::getline(std::cin, line))
    {
        inputStr += line;
        inputStr.push_back('\n');
    }

    if (inputStr.empty())
    {
        return 0;
    }

    Json::Value inputJson;
    Json::Reader reader;
    if (!reader.parse(inputStr, inputJson))
    {
        return 0;
    }

    Json::Value requests = inputJson["requests"];
    Json::Value responses = inputJson["responses"];

    Game game;

    const int turnCount = static_cast<int>(requests.size());
    const bool isBlackPlayer =
        turnCount > 0 &&
        requests[0u].get("x0", -1).asInt() == -1 &&
        requests[0u].get("y0", -1).asInt() == -1;

    game.setMyPlayer(isBlackPlayer ? Game::Player::kBlack : Game::Player::kWhite);

    // 根据 requests / responses 还原当前棋盘。
    const int reqCount = requests.size();
    const int resCount = responses.size();
    for (int i = 0; i < reqCount; ++i)
    {
        applyJsonMovePair(game, requests[i], game.getOppPlayer());
        if (i < resCount)
        {
            applyJsonMovePair(game, responses[i], game.getMyPlayer());
        }
    }

    game.initializeGlobalEvaluation();

    const bool singleStoneTurn = isBlackPlayer && responses.empty();

    int depth = readOptionInt(inputJson, "depth", 3);
    int width = readOptionInt(inputJson, "width", 10);
    depth = std::clamp(depth, 1, 6);
    width = std::clamp(width, 1, 30);

    MovePair bestMove = game.searchBestMovePair(depth, width, singleStoneTurn);

    Json::Value response;
    response["x0"] = bestMove.first.x;
    response["y0"] = bestMove.first.y;

    if (bestMove.singleStoneTurn || !game.isInBoard(bestMove.second.x, bestMove.second.y))
    {
        response["x1"] = -1;
        response["y1"] = -1;
    }
    else
    {
        response["x1"] = bestMove.second.x;
        response["y1"] = bestMove.second.y;
    }

    Json::Value outputJson;
    outputJson["response"] = response;

    Json::FastWriter fastWriter;
    std::cout << fastWriter.write(outputJson) << std::endl;
    return 0;
}
