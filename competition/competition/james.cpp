/*
Cloudict-derived Connect6 Botzone single-file port.
Original Cloudict copyright (c) 2008-2013 Hao Cui, Liang Li, Ruijian Wang, Siran Lin.
BSD-style license from the uploaded cloudict-master package.
Target: Botzone ConnectSix, 15x15, JSON interaction, single C++ file.
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <cctype>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
using namespace std;


static clock_t g_search_deadline_clock = 0;
static inline bool cloudict_time_up() {
    return g_search_deadline_clock != 0 && clock() >= g_search_deadline_clock;
}
static inline void cloudict_set_deadline_seconds(double seconds) {
    if (seconds <= 0) g_search_deadline_clock = 0;
    else g_search_deadline_clock = clock() + (clock_t)(seconds * CLOCKS_PER_SEC);
}

static const int BOTZONE_AB_DEPTH = 2;           // 1s-safe default. Test locally before raising to 3.
static const double BOTZONE_VCF_SECONDS = 0.30; // Cloudict VCF tactical budget.
static const double BOTZONE_AB_SECONDS = 0.78;  // Alpha-beta budget after VCF.



// ======================= defines.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */

#ifndef DEFINE_H_
#define DEFINE_H_



// Value of the Connect6 game
#define BOARD_SIZE          15          // Botzone ConnectSix board size.
#define GRID_NUM            (BOARD_SIZE + 2)  // 15*15 plus edges.
#define GRID_COUNT          (BOARD_SIZE * BOARD_SIZE)
#define BLACK               1           // Black flag in the board.
#define WHITE               2           // White flag in the board.
#define BORDER              3           // Border flag in the board.
#define NOSTONE             0           // Empty flag.

#define MSG_LENGTH          512

// Max values in the evaluation.
#define MAXINT              20000
#define MININT              -20000

// Control the depth of the search.
#define VCFDEPTH            14          // VCF default search depth.
#define ANTIVCFDEPTH        10          // Anti-VCF define search depth.
#define ANTIVCFN            5           // Anti-VCF define search depth.
// Control the width of the search
#define NUMOFONE            12           // tuned Botzone width first point.
#define NUMOFTWO            4           // tuned Botzone width second point.

// Point in the board.
typedef struct _stoneposition
{
    int x;
    int y;
} pos_t;

// One Move.
typedef struct _stonemove
{
    pos_t       positions[2];           // Point.
    double      score;                  // Score of the move.
} move_t;

// One point and its value.
typedef struct chess
{
    int x , y;
    int score;
}move_one_t;

extern int g_board_base_score[GRID_NUM-2][GRID_NUM-2];             // Base scores for each points in the board, defined in evaluation.cc

#endif


// ======================= tools.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */

#ifndef DEBUGPRINT_H_
#define DEBUGPRINT_H_


// Point (x, y) if in the valid position of the board.
#define IsValidPos(x,y)        ((x>0&&x<GRID_NUM-1 )&&(y>0&&y<GRID_NUM-1))

void init_board(char board[][GRID_NUM]);
bool is_win_by_premove(char board[][GRID_NUM], move_t* preMove);

void make_move(char board[][GRID_NUM], move_t* move, char color);
void unmake_move(char board[][GRID_NUM], move_t* move);

int log_to_file(char* msg);
int msg2move(char* msg, move_t* move);
int move2msg(move_t* move, char* msg);

int send_msg_to_slave(char* buf, int slave = 0);
int send_msg_to_master(char* buf);
int recv_msg_from_slave(char* buf,int len, int slave = 0);
int recv_msg_from_master(char* buf, int len);

int get_msg(char* buf, int maxLen);

void print_board(char tempboard[][GRID_NUM], move_t* preMove);
void print_score(move_one_t *moveList,int n);
void print_eval();

#endif


// ======================= evaluation.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */

#ifndef EVALUATION_H_
#define EVALUATION_H_


// Shape names.
#define huoer           0
#define huosan          1
#define miansan         2
#define huosi           3
#define miansi          4
#define huowu           5
#define mianwu          6
#define liu             7
#define qi              8
#define ba              9
#define jiu             10
#define shi             11
#define shiyi           12
#define sisi            13
#define wuxing          14

#define black_fix 15
#define white_fix 1

// Four directions
#define DUD             0           // Up down.
#define DLR             1           // Left righ.
#define DRU             2           // Right Up.
#define DRD             3           // Right Down.

// Scores for shapes.
#define Threat          300
#define CROSSPLUS       300
#define CROSSJIAN       300

// Score index for the shapes.
#define base_1          0           //2000
#define base_2          1           //1000
#define base_3          2           //0
#define base_4          3           //-1000
#define base_5          4           //-2000
#define base_6          5           //-3000
#define base_7          6           //-4000
#define huoer_big       7           //15
#define huoer_lit       8           //10
#define huosan_big      9           //25
#define huosan_lit      10          //20
#define miansan_big     11          //10
#define miansan_lit     12          //6
#define cross_big       13          //15
#define cross_lit       14          //10

class CEvaluation {
    public:
        CEvaluation();

        double evaluation( char ourOrder , char nextStep, char board[][GRID_NUM]);

    private:
        void set_situation(char board[][GRID_NUM]);

        void set_situation_for_one_direction(int x, int y ,short countx,short county,int dir,char board[][GRID_NUM]);

    public:
        double      m_time_evalution;

    private:
        //Evaluation
        int         m_w_situation[15];
        int         m_b_situation[15];
        int         m_w_cross;
        int         m_b_cross;
        int         m_w_detail[15][15];         // Detail information for the white in the board.
        int         m_b_detail[15][15];         // Detail information for the black.
        char        m_visited_direction[GRID_NUM][GRID_NUM][4];

        int         m_w_vir_detail[15][15];     // Virtual cross information for white.
        int         m_b_vir_detail[15][15];
        char        m_visited_virtual_direction[GRID_NUM][GRID_NUM][4];

        int         b_mean_point;
        int         w_mean_point;

};

#endif


// ======================= move_generator.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */

#ifndef MOVEGENERATER_H_
#define MOVEGENERATER_H_


class CSearchEngine;

class CMoveGenerator {
    public:
        CMoveGenerator();

        int get_move_list(char ourColor , move_t* moveList, char board[][GRID_NUM]);

    private:
        int init_valuable_space(char board[][GRID_NUM]);
        int sort_merge(move_one_t list[],move_one_t listOne[],int oneN,move_one_t listTwo[],int twoN);
        bool extend_pos(char x, char y, char board[][GRID_NUM]);
        void add_new_pos_for_two(char x, char y);
        void add_new_pos_for_two_special(char x, char y);

        int set_score( char ourColor , int step , move_one_t moveList[] , char board[][GRID_NUM]  );
        int set_score_single ( char ourColor, int x, int y, int step, char board[][GRID_NUM] );
        int set_by_direction1 ( char color, int x, int y, int step, char board[][GRID_NUM] );
        int set_by_direction2 ( char color, int x, int y, int step, char board[][GRID_NUM] );
        int set_by_direction3 ( char color, int x, int y, int step, char board[][GRID_NUM] );
        int set_by_direction4 ( char color, int x, int y, int step, char board[][GRID_NUM] );

    public:
        double m_time_get_moves;
        double m_time_set_score;
        double m_time_test;

    private:
        int m_dead_four_plus;
        std::vector<pos_t> m_pos_to_update;
        std::vector<pos_t> m_pos_to_update_special;

        int map[GRID_NUM][GRID_NUM];
};

#endif


// ======================= search_engine.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


class CSearchEngine {
    public:
        CSearchEngine();

        void before_search(char board[][GRID_NUM], char color, int m_alphabeta_depth);
        double alpha_beta_search(int depth,double alpha,double beta,char ourColor, move_t* bestMove,move_t* preMove);

    private:

    public:
        int m_total_nodes;

    private:
        char                m_board[GRID_NUM][GRID_NUM];                    // The board in the search engine.
        char                m_chess_type;
        int                 m_alphabeta_depth;
        CMoveGenerator      m_move_gernerator;
        CEvaluation         m_evaluator;

};


// ======================= pattern.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */

#ifndef PATTERN_H
#define PATTERN_H


//Pattern
typedef struct attrib
{
    int offset[2];
    /* mode */
    int mode;
} attrib_t;


/* DFA state. */
typedef struct state
{
    int att;
    int next[4];
} state_t;

/* DFA. */
typedef struct dfa
{
    /* File header. */
    char name[80];

    /* Transition graph. */
    state_t *states;
    int max_states;
    int last_state;

    /* Attributes sets. */
    attrib_t *indexes;
    int max_indexes;
    int last_index;
} dfa_t;

class CDFA {
    public:
        CDFA();
        /* Attribute list. */
        bool dfa_init();

        int pattern_match(char ourColor, move_t bestMove[], char board[][GRID_NUM]);

    private:
        bool dfa_create(dfa_t *pdfa, char str[]);
        void dfa_kill();
        void dfa_resize(dfa_t *pdfa, int max_states, int max_indexes);
        int change(int Color);
        int check(move_t bestMove[], move_t now);
        void new_match2(pos_t point, dfa_t *pdfa, move_t bestMove[], int direction);
        void new_match(pos_t point, move_t bestMove[], int ori_direction);
        void addpoint(move_t bestMove[], pos_t point);
        void match2(pos_t point, dfa_t *pdfa, move_t bestMove[], int direction, int dfa_num);
        void match(pos_t point, int direction, move_t * bestMove);
        int find(char temp);

    private:
        char    (*m_board)[GRID_NUM];
        int     m_dfa_index;
        dfa_t   m_dfa_array[10000];

        int     count, sim_c, my_color;
        FILE    *m_partin;

};

#endif


// ======================= vcf_search.h =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */

#ifndef VCFSEARCH_H_
#define VCFSEARCH_H_



#define HASHSIZE 19997

//VCFSearch
typedef struct _node
{
    pos_t p1;
    pos_t p2;
    int pre, next;
}node_t;

typedef struct _listnode
{
    int pos;
    int score;
    int dist;
}ListNode;

typedef struct _hashnode
{
    int dep;
    unsigned long hash;
    int pre;
    move_t move;
    bool res;
    char color;
}HashNode;

class CVCFSearch {
    public:
        CVCFSearch();
        CVCFSearch(char* ptr_board[GRID_NUM], char* ptr_chess_type);

        // Check if VCFSearch is needed.
        bool vcf_judge(move_t * preMove);

        int init();

        void init_game();

        void before_search(char board[][GRID_NUM], char color);

        // VCFSearch implements.
        bool vcf_search(int depth,char ourColor,move_t * bestMove,move_t * preMove, int preNode, int prePos);

        // Anti search for VCF.
        bool anti_vcf_search(int depth,char ourColor,move_t * bestMove,move_t * preMove, int preNode, int prePos);

    private:
        // Get move list for VCF.
        int vcf_get_move_list( char ourColor,char a_d, pos_t * canUse, int n_Pos, move_t * moveList, move_t* pretMove);

        // Check the move if thread.
        int is_attack(char board[][GRID_NUM],char Color, move_t * Move);

        // Check the move can form a connected four.
        int is_dlb_attack(char board[][GRID_NUM],char Color, move_t * Move);
        int vcf_hash_check(HashNode node);
        unsigned long vcf_hash_board(char board[GRID_NUM][GRID_NUM]);

        int is_three(char position[GRID_NUM][GRID_NUM],char Color, pos_t * Pos);
        void sort(move_t * moveList, int n_moveList, move_t * preMove);
        inline int vcf_abs(int a);
        inline int dist(pos_t p1, pos_t p2, pos_t pt);
        inline int vcf_min(int a, int b);

    public:
        int                 m_vcf_node;

    private:
        CDFA                m_dfa;

        char                m_board[GRID_NUM][GRID_NUM];
        char                m_chess_type;
        char                m_has_win;

        char                m_org_board[GRID_NUM][GRID_NUM];
        ListNode            m_list_node[10000];
        move_t              m_tmp_move_list[10000];
        move_t              m_vcf_move_list[VCFDEPTH+1][10000];                         // Generated move list.
        int                 m_hash_head[VCFDEPTH+1][HASHSIZE];
        int                 m_hash_next[1000000];
        HashNode            m_hash_que[1000000];

        node_t              m_vcf_move_table[100000];
        int                 m_vcf_total_node;
        int                 m_vcf_now_pos;
        int                 m_dy[4];
        int                 m_dx[4];                                                    // Directions.
        char                m_vcf_use[GRID_NUM][GRID_NUM][4];                           // Can be used position in the board.
        char                m_vcf_mark[GRID_NUM][GRID_NUM];

};

#endif


// ======================= evaluation.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */



int g_board_base_score[GRID_NUM-2][GRID_NUM-2] =
{
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,2,3,3,3,3,3,3,3,3,3,2,1,0},
    {0,1,2,3,4,4,4,4,4,4,4,3,2,1,0},
    {0,1,2,3,4,5,5,5,5,5,4,3,2,1,0},
    {0,1,2,3,4,5,6,6,6,5,4,3,2,1,0},
    {0,1,2,3,4,5,6,8,6,5,4,3,2,1,0},
    {0,1,2,3,4,5,6,6,6,5,4,3,2,1,0},
    {0,1,2,3,4,5,5,5,5,5,4,3,2,1,0},
    {0,1,2,3,4,4,4,4,4,4,4,3,2,1,0},
    {0,1,2,3,3,3,3,3,3,3,3,3,2,1,0},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

int dna[16] = {2000 , 1000, 0 ,-1000 ,-2000 ,-3000 ,-4000 ,15 ,10 ,25 ,20, 10, 6 ,15 , 10,0};

CEvaluation::CEvaluation()
{
}

double CEvaluation::evaluation( char ourOrder , char nextStep, char board[][GRID_NUM])
{
    double score = 0;
    int i,j = 0;
    clock_t beg,end;
    beg = clock();

    set_situation(board);

    // Odd level evaluation
    if ( ourOrder == nextStep )
    {
        // Our is black
        if ( ourOrder == BLACK )
        {
            if (m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[miansi] + m_b_situation[mianwu] >= 1 || (m_b_situation[liu]+m_b_situation[qi]+m_b_situation[ba]+m_b_situation[jiu]!=0))
            {
                return MAXINT -1;
            }
            else
                if ( ( m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[mianwu] + m_w_situation[miansi] >= 3 ) )
                {
                    return 0;
                }
                else
                {
                    if(m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[mianwu] + m_w_situation[miansi] == 2)  //7-5
                    {
                        if(m_w_situation[huosan]>=2||(m_w_situation[huosan]==1&&m_w_situation[miansan]>=1))
                        {
                            return 1;
                        }
                        else
                        {
                            if(m_w_situation[huosan]>=1)   //7
                            {
                                score = dna[base_7];
                                score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                            }
                            else
                            {
                                if(m_w_situation[huoer]>=2&&(m_w_detail[huoer][huosan]+m_w_detail[huoer][huoer]+m_w_detail[huoer][miansan] + m_w_detail[huoer][huosi] + m_w_detail[huoer][sisi] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] )>0)  //7
                                {
                                    score = dna[base_7];
                                    score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                    score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                }
                                else
                                {
                                    if (m_w_situation[huoer]>=1&&(m_w_detail[huoer][huosan]+m_w_detail[huoer][huoer]+m_w_detail[huoer][miansan] + m_w_detail[huoer][huosi] + m_w_detail[huoer][sisi] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] )>0)
                                    {
                                        score = dna[base_7];
                                        score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                        score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                    }
                                    else
                                    {
                                        if( m_w_situation[miansan] >= 2 && ( m_w_detail[miansan][sisi] + m_w_detail[miansan][miansan] + m_w_vir_detail[miansan][miansan] + m_w_vir_detail[huoer][miansan] ) > 0 ) //7
                                        {
                                            score = dna[base_7];
                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                        }
                                        else
                                        {
                                            if(m_w_situation[miansan]>=2)   //6
                                         {
                                             score = dna[base_6];
                                             score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                             score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                         }
                                        else
                                         {
                                             if( m_w_situation[huoer]>=1 )  //6
                                             {
                                                 score = dna[base_6];
                                                 score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                 score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                             }
                                             else //5
                                             {
                                                 if ( m_w_situation[miansan] > 0 && m_w_situation[huoer] == 0 )
                                                 {
                                                     if ( m_b_situation[miansan] >= m_w_situation[miansan] && m_b_situation[huoer] + m_b_situation[huosan] >= 3 && m_b_detail[huoer][huoer] + m_b_detail[huoer][miansan] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] > 0 )
                                                     {
                                                         score = dna[base_2];
                                                         score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                         score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                     }
                                                     else
                                                     {
                                                         if ( m_b_situation[miansan] >= m_w_situation[miansan] && m_b_situation[huoer] + m_b_situation[huosan] == 2 && m_b_detail[huoer][huoer] + m_b_detail[huoer][miansan] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] > 0 )
                                                         {
                                                             if ( m_b_situation[huosan] > 0 )
                                                             {
                                                                 score = dna[base_2];
                                                             }
                                                             else
                                                             {
                                                                 score = dna[base_3];
                                                             }
                                                             score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                             score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                         }
                                                         else
                                                         {
                                                             score = dna[base_6];
                                                             score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                             score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                         }
                                                     }
                                                 }
                                                 else
                                                 {
                                                     if ( m_w_situation[miansan] > 0 )
                                                     {
                                                         score = dna[base_6];
                                                         score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                         score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                     }
                                                     else
                                                     {
                                                         score = dna[base_5];
                                                         score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                         score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                     }
                                                 }
                                             }
                                         }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if(m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[mianwu] + m_w_situation[miansi] == 1)
                        {
                            if(m_b_situation[huosan]>=2)  //2
                            {
                                score = dna[base_1];
                                score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                            }
                            else
                            {
                                if ( m_b_situation[huosan] == 1 )
                                {
                                    if ( m_b_situation[huoer] >= 2 && ( m_b_detail[huoer][huoer] + m_b_detail[huoer][miansan] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] ) > 0 )
                                    {
                                        score = dna[base_1];
                                    }
                                    else
                                    {
                                        if ( m_b_situation[miansan] >= 2 && ( m_b_detail[miansan][miansan] + m_b_vir_detail[miansan][miansan] + m_b_detail[huoer][miansan] + m_b_vir_detail[huoer][miansan] ) > 0 )
                                        {
                                            score = dna[base_1];
                                        }
                                        else
                                        {
                                            if ( m_b_situation[huoer] > 0 || m_b_situation[miansan] >= 2 )
                                            {
                                                score = dna[base_3];
                                            }
                                            else
                                            {
                                                score = dna[base_5];
                                            }
                                        }
                                    }
                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                }
                                else
                                {
                                    if ( m_w_situation[huosan] >= 2 )
                                    {
                                        return 1;
                                    }
                                    else
                                    {
                                        if ( m_w_situation[huosan] == 1 )
                                        {
                                            if ( m_w_situation[huoer] > 0 )
                                            {
                                                score = dna[base_7];
                                                score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                            }
                                            else
                                            {
                                                if ( m_w_situation[huosan] + m_w_situation[miansan] <= 1 )
                                                {
                                                    score = dna[base_3];
                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    if ( m_b_situation[miansan] - m_w_situation[miansan] - m_w_situation[huosan] >= 0 && m_b_situation[huoer] >= 2 )
                                                    {
                                                        score = dna[base_3];
                                                        score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                    }
                                                    else
                                                    {
                                                        if ( m_b_situation[miansan] < m_w_situation[huosan] + m_w_situation[miansan] )
                                                        {
                                                            score = dna[base_6];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                        else
                                                        {
                                                            score = dna[base_5];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if(m_b_situation[miansan]>=1)  //3
                                            {
                                                if ( m_b_situation[miansan] > m_w_situation[miansan] && m_b_situation[huoer] > 0 )
                                                {
                                                    score = dna[base_2];
                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    if ( m_b_situation[miansan] >= m_w_situation[miansan] )
                                                    {
                                                        score = dna[base_3];
                                                        score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                    }
                                                    else
                                                    {
                                                        if ( m_w_situation[miansan] > m_b_situation[miansan] && m_w_situation[huoer] > 0 )
                                                        {
                                                            score = dna[base_7];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                        else
                                                        {
                                                            score = dna[base_5];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                    }
                                                }


                                            }
                                            else
                                            {
                                                if ( m_w_situation[huoer] >= 2 )
                                                {
                                                    score = dna[base_7];score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                    score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                }
                                                else
                                                {
                                                    if ( m_w_situation[miansan] >= 2 && m_b_situation[huoer] == 0 )
                                                    {
                                                        score = dna[base_6];
                                                        score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                        score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                    }
                                                    else
                                                    {
                                                        if ( m_w_situation[miansan] == 1 && m_w_situation[huoer] == 1 )
                                                        {
                                                            score = dna[base_6];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                        else
                                                        {
                                                            score = dna[base_5];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if(m_b_situation[huosan]>=1) //1
                            {
                                score=dna[base_1];
                                score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                            }
                            else
                            {
                                if(m_b_situation[huoer] >= 2 && (m_b_detail[huoer][huoer] + m_b_detail[huoer][miansan] + m_b_detail[huoer][sisi] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] ) > 0 )  //1
                                {
                                    score = dna[base_1];
                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                }
                                else
                                {
                                    if ( m_b_situation[miansan] - m_w_situation[miansan] >= 1 && m_b_detail[huoer][huoer] + m_b_detail[huoer][miansan] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] > 0 && m_w_situation[huosan] == 0 && m_w_vir_detail[huoer][miansan] == 0 )
                                    {
                                        score = dna[base_3];
                                        score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                        score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                    }
                                    else
                                    {
                                        if(m_b_situation[huoer]==1 &&( m_b_detail[huoer][sisi] +m_b_detail[huoer][miansan] + m_b_detail[huoer][miansan] + m_b_vir_detail[huoer][miansan])> 0) //2
                                        {
                                            score = dna[base_2];
                                            score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                            score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                        }
                                        else
                                        {
                                            if( m_b_situation[miansan] >= 2 && m_b_detail[miansan][miansan] + m_b_vir_detail[miansan][miansan] > 0 )  //2
                                            {
                                                score = dna[base_2];
                                                score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                            }
                                            else
                                            {
                                                if ( m_b_situation[huoer] >= 1 )  //3
                                                {
                                                    score = dna[base_3];
                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    score = dna[base_5];
                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                }
                                            }
                                        }
                                    }

                                }
                            }
                        }

                        for(i=1;i<=BOARD_SIZE;i++)
                            for(j=1;j<=BOARD_SIZE;j++)
                            {
                                if ( board[i][j] == ourOrder )
                                {
                                    score += g_board_base_score[i-1][j-1];
                                }
                                if ( board[i][j] == 2 )
                                {
                                    score -= g_board_base_score[i-1][j-1];
                                }
                            }
                    }
                }
        }
        // Our is white.
        else
        {
            if (m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[miansi] + m_w_situation[mianwu] >= 1 || (m_w_situation[liu]+m_w_situation[qi]+m_w_situation[ba]+m_w_situation[jiu]!=0))
            {
                return MAXINT -1;
            }
            else
                if ( ( m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[mianwu] + m_b_situation[miansi] >= 3 ) )
                {
                    return 0;
                }
                else
                {
                    if(m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[mianwu] + m_b_situation[miansi] == 2)  //7-5
                    {
                        if(m_b_situation[huosan]>=2||(m_b_situation[huosan]==1&&m_b_situation[miansan]>=1))
                        {
                            return 1;
                        }
                        else
                        {
                            if(m_b_situation[huosan]>=1)   //7
                            {
                                score = dna[base_7];
                                score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                            }
                            else
                            {
                                if(m_b_situation[huoer]>=2&&(m_b_detail[huoer][huosan]+m_b_detail[huoer][huoer]+m_b_detail[huoer][miansan] + m_b_detail[huoer][huosi] + m_b_detail[huoer][sisi] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] )>0)  //7
                                {
                                    score = dna[base_7];
                                    score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                    score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                }
                                else
                                {
                                    if (m_b_situation[huoer]>=1&&(m_b_detail[huoer][huosan]+m_b_detail[huoer][huoer]+m_b_detail[huoer][miansan] + m_b_detail[huoer][huosi] + m_b_detail[huoer][sisi] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan] )>0)
                                    {
                                        score = dna[base_7];
                                        score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                        score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                    }
                                    else
                                    {
                                        if( m_b_situation[miansan] >= 2 && ( m_b_detail[miansan][sisi] + m_b_detail[miansan][miansan] + m_b_vir_detail[miansan][miansan] + m_b_vir_detail[huoer][miansan] ) > 0 ) //7
                                        {
                                            score = dna[base_7];
                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                        }
                                        else
                                        {
                                            if(m_b_situation[miansan]>=2)   //6
                                         {
                                             score = dna[base_6];
                                             score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                             score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                         }
                                            else
                                         {
                                             if( m_b_situation[huoer]>=1 )  //6
                                             {
                                                 score = dna[base_6];
                                                 score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                 score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                             }
                                             else //5
                                             {
                                                 if ( m_b_situation[miansan] > 0 && m_b_situation[huoer] == 0 )
                                                 {
                                                     if ( m_w_situation[miansan] >= m_b_situation[miansan] && m_w_situation[huoer] + m_w_situation[huosan] >= 3 && m_w_detail[huoer][huoer] + m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] > 0 )
                                                     {
                                                         score = dna[base_2];
                                                         score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                         score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                     }
                                                     else
                                                     {
                                                         if ( m_w_situation[miansan] >= m_b_situation[miansan] && m_w_situation[huoer] + m_w_situation[huosan] == 2 && m_w_detail[huoer][huoer] + m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] > 0 )
                                                         {
                                                             if ( m_w_situation[huosan] > 0 )
                                                             {
                                                                 score = dna[base_2];
                                                             }
                                                             else
                                                             {
                                                                 score = dna[base_3];
                                                             }
                                                             score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                             score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                         }
                                                         else
                                                         {
                                                             score = dna[base_6];
                                                             score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                             score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                         }
                                                     }
                                                 }
                                                 else
                                                 {
                                                     if ( m_b_situation[miansan] > 0 )
                                                     {
                                                         score = dna[base_6];
                                                         score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                         score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                     }
                                                     else
                                                     {
                                                         score = dna[base_5];
                                                         score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                         score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                     }
                                                 }
                                             }
                                         }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if(m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[mianwu] + m_b_situation[miansi] == 1)
                        {
                            if(m_w_situation[huosan]>=2)  //2
                            {
                                score = dna[base_1];
                                score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                            }
                            else
                            {
                                if ( m_w_situation[huosan] == 1 )
                                {
                                    if ( m_w_situation[huoer] >= 2 && ( m_w_detail[huoer][huoer] + m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] ) > 0 )
                                    {
                                        score = dna[base_1];
                                    }
                                    else
                                    {
                                        if ( m_w_situation[miansan] >= 2 && ( m_w_detail[miansan][miansan] + m_w_vir_detail[miansan][miansan] + m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][miansan] ) > 0 )
                                        {
                                            score = dna[base_1];
                                        }
                                        else
                                        {
                                            if ( m_w_situation[huoer] > 0 || m_w_situation[miansan] >= 2 )
                                            {
                                                score = dna[base_3];
                                            }
                                            else
                                            {
                                                score = dna[base_5];
                                            }
                                        }
                                    }
                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                }
                                else
                                {
                                    if ( m_b_situation[huosan] >= 2 )
                                    {
                                        return 1;
                                    }
                                    else
                                    {
                                        if ( m_b_situation[huosan] == 1 )
                                        {
                                            if ( m_b_situation[huoer] > 0 )
                                            {
                                                score = dna[base_7];
                                                score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                            }
                                            else
                                            {
                                                if ( m_b_situation[huosan] + m_b_situation[miansan] <= 1 )
                                                {
                                                    score = dna[base_3];
                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    if ( m_w_situation[miansan] - m_b_situation[miansan] - m_b_situation[huosan] >= 0 && m_w_situation[huoer] >= 2 )
                                                    {
                                                        score = dna[base_3];
                                                        score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                    }
                                                    else
                                                    {
                                                        if ( m_w_situation[miansan] < m_b_situation[huosan] + m_b_situation[miansan] )
                                                        {
                                                            score = dna[base_6];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        }
                                                        else
                                                        {
                                                            score = dna[base_5];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if(m_w_situation[miansan]>=1)  //3
                                            {
                                                if ( m_w_situation[miansan] > m_b_situation[miansan] && m_w_situation[huoer] > 0 )
                                                {
                                                    score = dna[base_2];
                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    if ( m_w_situation[miansan] >= m_b_situation[miansan] )
                                                    {
                                                        score = dna[base_3];
                                                        score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                    }
                                                    else
                                                    {
                                                        if ( m_b_situation[miansan] > m_w_situation[miansan] && m_b_situation[huoer] > 0 )
                                                        {
                                                            score = dna[base_7];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        }
                                                        else
                                                        {
                                                            score = dna[base_5];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        }
                                                    }
                                                }


                                            }
                                            else
                                            {
                                                if ( m_b_situation[huoer] >= 2 )
                                                {
                                                    score = dna[base_7];
                                                    score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                    score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                }
                                                else
                                                {
                                                    if ( m_b_situation[miansan] >= 2 && m_w_situation[huoer] == 0 )
                                                    {
                                                        score = dna[base_6];
                                                        score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                        score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                    }
                                                    else
                                                    {
                                                        if ( m_b_situation[miansan] == 1 && m_b_situation[huoer] == 1 )
                                                        {
                                                            score = dna[base_6];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        }
                                                        else
                                                        {
                                                            score = dna[base_5];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if(m_w_situation[huosan]>=1) //1
                            {
                                score=dna[base_1];
                                score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                            }
                            else
                            {
                                if(m_w_situation[huoer] >= 2 && (m_w_detail[huoer][huoer] + m_w_detail[huoer][miansan] + m_w_detail[huoer][sisi] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] ) > 0 )  //1
                                {
                                    score = dna[base_1];
                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                }
                                else
                                {
                                    if ( m_w_situation[miansan] - m_b_situation[miansan] >= 1 && m_w_detail[huoer][huoer] + m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan] > 0 && m_b_situation[huosan] == 0 && m_b_vir_detail[huoer][miansan] == 0 )
                                    {
                                        score = dna[base_3];
                                        score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                        score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                    }
                                    else
                                    {
                                        if(m_w_situation[huoer]==1 &&( m_w_detail[huoer][sisi] +m_w_detail[huoer][miansan] + m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][miansan])> 0) //2
                                        {
                                            score = dna[base_2];
                                            score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                            score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                        }
                                        else
                                        {
                                            if( m_w_situation[miansan] >= 2 && m_w_detail[miansan][miansan] + m_w_vir_detail[miansan][miansan] > 0 )  //2
                                            {
                                                score = dna[base_2];
                                                score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                            }
                                            else
                                            {
                                                if ( m_w_situation[huoer] >= 1 )  //3
                                                {
                                                    score = dna[base_3];
                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    score = dna[base_5];
                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                }
                                            }
                                        }
                                    }

                                }
                            }
                        }

                        for(i=1;i<=BOARD_SIZE;i++)
                            for(j=1;j<=BOARD_SIZE;j++)
                            {
                                if ( board[i][j] == ourOrder )
                                {
                                    if ( board[i][j] == ourOrder )
                                    {
                                        score += g_board_base_score[i-1][j-1];
                                    }
                                    if ( board[i][j] == 1 )
                                    {
                                        score -= g_board_base_score[i-1][j-1];
                                    }
                                }
                            }
                    }

                }
        }
        score += MAXINT/2;
    }
    // Odd level evaluation.
    else
    {
        // Our is white
        if ( ourOrder == WHITE )
        {
            if ( ( m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[miansi] + m_b_situation[mianwu] >= 1 || (m_b_situation[liu]+m_b_situation[qi]+m_b_situation[ba]+m_b_situation[jiu]!=0)) )
            {
                return 0;
            }
            else
                if (m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[mianwu] + m_w_situation[miansi] >= 3 || (m_w_situation[liu]+m_w_situation[qi]+m_w_situation[ba]+m_w_situation[jiu]!=0))
                {
                    return MAXINT - 1;
                }
                else
                {
                    if(m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[mianwu] + m_w_situation[miansi] == 2)
                    {
                        if(m_w_situation[huosan]>=1)   //1
                        {
                            score = dna[base_1];
                            score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                            score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                        }
                        else
                        {
                            if(m_w_situation[huoer]>=2&&(m_w_detail[huoer][huosan]+m_w_detail[huoer][huoer]+m_w_detail[huoer][miansan] + m_w_detail[huoer][huosi] + m_w_detail[huoer][sisi]  + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan])>0)  //1
                            {
                                score = dna[base_1];
                                score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                            }
                            else
                            {
                                if ( m_w_situation[huoer] >= 2 )  //2
                                {
                                    score = dna[base_3];
                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                }
                                else
                                {
                                    if( m_w_situation[huoer]==1 && ( m_w_detail[huoer][huosan]+m_w_detail[huoer][huoer]+m_w_detail[huoer][miansan] + m_w_detail[huoer][huosi] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan]) >0 )  //2
                                    {
                                        score = dna[base_2];
                                        score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                        score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                    }
                                    else
                                    {
                                        if(m_w_situation[miansan]>=2&&(m_w_detail[miansan][sisi]+m_w_detail[miansan][miansan])>0) //2
                                        {
                                            score = dna[base_2];
                                            score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                            score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                        }
                                        else
                                        {
                                            if( m_w_situation[miansan] - m_b_situation[miansan] >= 1 && m_b_situation[huosan] == 0 && m_b_vir_detail[huoer][miansan] == 0 )   //3
                                            {
                                                score = dna[base_3];
                                                score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                            }
                                            else
                                            {
                                                if ( m_w_situation[huoer] == 1 )
                                                {
                                                    score = dna[base_3];
                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                }


                                                else
                                                {
                                                    score = dna[base_6];
                                                    score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                    score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];


                                                }
                                            }

                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                        if(m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[mianwu] + m_w_situation[miansi] == 1)
                        {
                            if(m_b_situation[huosan]>=1)  //7
                            {
                                score = dna[base_7];
                                if(m_b_situation[huoer]>=3&&m_b_detail[huoer][huoer]>0)
                                {
                                    score -= m_b_situation[huoer]* 40;
                                }
                                else
                                {
                                    if(m_b_situation[huoer]>=2&&m_b_detail[huoer][huoer]>0)
                                    {
                                        score -= m_b_situation[huoer] * 30;
                                    }
                                    else
                                    {
                                        score -= m_b_situation[huoer] * 20;
                                    }
                                }
                                score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];


                            }
                            else
                            {
                                if(m_b_situation[miansan]>=1)
                                {
                                    if ( ( m_w_situation[miansan] >= m_b_situation[miansan] && m_w_situation[huoer] + m_w_situation[huosan] >= 2 ) && ( m_w_detail[huoer][miansi] + m_w_detail[huosan][miansi] + m_w_detail[huoer][huoer] + m_w_vir_detail[huoer][huoer]> 0 ) )
                                    {
                                        if ( m_b_vir_detail[huoer][miansan] > 0 )
                                        {
                                            score = dna[base_7];
                                            score += m_w_situation[huosan] * 20 + m_w_situation[huoer] * 10 + m_w_situation[miansan] * 6 +m_w_cross * 10;
                                            score -= m_b_situation[huosan] * 25 + m_b_situation[huoer] * 15 + m_b_situation[miansan] * 10 +m_b_cross * 15;
                                        }
                                        else
                                        {
                                            score = dna[base_2];
                                            score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                            score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                        }
                                    }
                                    else
                                    {
                                        score = dna[base_6];
                                        score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                        score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                    }


                                }
                                else        //2
                                {
                                    if ( m_w_situation[miansan] + m_w_situation[huoer] + m_w_situation[huosan] >= 2 )
                                    {
                                        score = dna[base_2];
                                    }
                                    else
                                    {
                                        if ( m_w_situation[miansan] + m_w_situation[huoer] + m_w_situation[huosan] == 1 )
                                        {
                                            score = dna[base_3];
                                        }
                                        else
                                        {
                                            score = dna[base_6];
                                        }
                                    }
                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                }
                            }

                        }
                        else
                        {
                            if(m_b_situation[huosan]>=1) //7
                            {
                                score = dna[base_7];
                                if(m_b_situation[huoer]>=3&&m_b_detail[huoer][huoer]>0)
                                {
                                    score -= m_b_situation[huoer] *40;
                                }
                                else
                                {
                                    score -= m_b_situation[huoer] *20;
                                }
                                if(m_b_situation[huosan]>=2&&(m_b_detail[huoer][huosan]+m_b_detail[huoer][huoer])>0)
                                {
                                    score -= m_b_situation[huosan] *50;
                                }
                                else
                                {
                                    score -= m_b_situation[huosan] *40;
                                }
                                score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                            }
                            else
                            {
                                if(m_b_situation[huoer] >= 2 && (m_b_detail[huoer][huoer] + m_b_detail[huoer][miansan] + m_b_detail[huoer][sisi] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan])>0)  //7
                                {
                                    score = dna[base_7];
                                    score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                    score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                }
                                else
                                {
                                    if( m_b_situation[huoer] >=2 ) //6
                                    {
                                        score = dna[base_6];
                                        score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                        score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                    }
                                    else
                                    {
                                        if(m_b_situation[huoer]>=1 &&( m_b_detail[huoer][sisi] +m_b_detail[huoer][miansan])> 0 + m_b_vir_detail[huoer][miansan]) //6
                                        {
                                            score = dna[base_7];
                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                        }
                                        else
                                        {
                                            if(m_b_situation[miansan] >=2 &&m_b_detail[miansan][miansan] >0)  //6
                                            {
                                                score = dna[base_7];
                                                score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                            }
                                            else
                                            {
                                                if ( m_b_situation[huoer] >= 1 )
                                                {
                                                    score = dna[base_6];
                                                    score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                    score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                                }
                                                else
                                                {
                                                    if ( m_b_situation[miansan] >= 1 && m_b_situation[huoer] == 0 )
                                                    {
                                                        if ( ( m_w_situation[miansan] >= m_b_situation[miansan] && m_w_situation[huoer] + m_w_situation[huosan] > 2 ) && ( m_w_detail[huoer][miansi] + m_w_detail[huosan][miansi] + m_w_detail[huoer][huoer] + m_w_vir_detail[huoer][huoer]> 0 ) )
                                                        {
                                                            score = dna[base_2];
                                                            score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                            score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                        }
                                                        else
                                                        {
                                                            if ( ( m_w_situation[miansan] >= m_b_situation[miansan] && m_w_situation[huoer] + m_w_situation[huosan] == 2 ) && ( m_w_detail[huoer][miansi] + m_w_detail[huosan][miansi] + m_w_detail[huoer][huoer] + m_w_vir_detail[huoer][huoer]> 0 ) )
                                                            {
                                                                if ( m_w_situation[huosan] > 0 )
                                                                {
                                                                    score = dna[base_2];
                                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                                }
                                                                else
                                                                {
                                                                    score = dna[base_3];
                                                                    score += m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                                    score -= m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                                }
                                                            }
                                                            else
                                                            {
                                                                score = dna[base_6];
                                                                score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                                score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if ( m_b_situation[miansan] > 0 )
                                                        {
                                                            score = dna[base_6];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                                        }
                                                        else    // 3
                                                        {
                                                            score = dna[base_5];
                                                            score += m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                            score -= m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];

                                                        }
                                                    }
                                                }
                                            }

                                        }
                                    }
                                }
                            }
                        }

                        for(i=1;i<=BOARD_SIZE;i++)
                            for(j=1;j<=BOARD_SIZE;j++)
                            {
                                if ( board[i][j] == ourOrder )
                                {
                                    if ( board[i][j] == ourOrder )
                                    {
                                        score += g_board_base_score[i-1][j-1];
                                    }
                                    if ( board[i][j] == 1 )
                                    {
                                        score -= g_board_base_score[i-1][j-1];
                                    }
                                }
                            }
                }

        }
        // Our is black.
        else
        {
#ifdef _DEBUG_
            print_eval();
#endif
            if ((m_w_situation[huosi]*2 + m_w_situation[huowu]*2 + m_w_situation[miansi] + m_w_situation[mianwu] >= 1 || (m_w_situation[liu]+m_w_situation[qi]+m_w_situation[ba]+m_w_situation[jiu]!=0)))
            {
                return 0;
            }
            else
                if (m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[mianwu] + m_b_situation[miansi] >= 3 || (m_b_situation[liu]+m_b_situation[qi]+m_b_situation[ba]+m_b_situation[jiu]!=0))
                {
                    return MAXINT - 1;
                }
                else
                {
                    if(m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[mianwu] + m_b_situation[miansi] == 2)
                    {
                        if(m_b_situation[huosan]>=1)   //1
                        {
                            score = dna[base_1];
                            score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                            score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                        }
                        else
                        {
                            if(m_b_situation[huoer]>=2&&(m_b_detail[huoer][huosan]+m_b_detail[huoer][huoer]+m_b_detail[huoer][miansan] + m_b_detail[huoer][huosi] + m_b_detail[huoer][sisi] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan])>0)  //1
                            {
                                score = dna[base_1];
                                score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                            }
                            else
                            {
                                if ( m_b_situation[huoer] >= 2 )  //2
                                {
                                    score = dna[base_3];
                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                }
                                else
                                {
                                    if( m_b_situation[huoer]==1 && ( m_b_detail[huoer][huosan]+m_b_detail[huoer][huoer]+m_b_detail[huoer][miansan] + m_b_detail[huoer][huosi] + m_b_vir_detail[huoer][huoer] + m_b_vir_detail[huoer][miansan]) >0 )  //2
                                    {
                                        score = dna[base_2];
                                        score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                        score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                    }
                                    else
                                    {
                                        if(m_b_situation[miansan]>=2&&(m_b_detail[miansan][sisi]+m_b_detail[miansan][miansan])>0) //2
                                        {
                                            score = dna[base_2];
                                            score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                            score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                        }
                                        else
                                        {
                                            if( m_b_situation[miansan] - m_w_situation[miansan] >= 1 && m_w_situation[huosan] == 0 && m_w_vir_detail[huoer][miansan] == 0 )   //3
                                            {
                                                score = dna[base_3];
                                                score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                            }
                                            else
                                            {
                                                if ( m_b_situation[huoer] == 1 )
                                                {
                                                    score = dna[base_3];
                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                }
                                                else
                                                {
                                                    score = dna[base_6];
                                                    score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                    score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                }

                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                        if(m_b_situation[huosi]*2 + m_b_situation[huowu]*2 + m_b_situation[mianwu] + m_b_situation[miansi] == 1)
                        {
                            if(m_w_situation[huosan]>=1)  //7
                            {
                                score = dna[base_7];
                                if(m_w_situation[huoer]>=3&&m_w_detail[huoer][huoer]>0)
                                {
                                    score -= m_w_situation[huoer]* 40;
                                }
                                else
                                {
                                    if(m_w_situation[huoer]>=2&&m_w_detail[huoer][huoer]>0)
                                    {
                                        score -= m_w_situation[huoer] * 30;
                                    }
                                    else
                                    {
                                        score -= m_w_situation[huoer] * 20;
                                    }
                                }
                                score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                            }
                            else
                            {
                                if(m_w_situation[miansan]>=1)
                                {
                                    if ( ( m_b_situation[miansan] >= m_w_situation[miansan] && m_b_situation[huoer] + m_b_situation[huosan] >= 2 ) && ( m_b_detail[huoer][miansi] + m_b_detail[huosan][miansi] + m_b_detail[huoer][huoer] + m_b_vir_detail[huoer][huoer]> 0 ) )
                                    {
                                        if ( m_w_vir_detail[huoer][miansan] > 0 )
                                        {
                                            score = dna[base_7];
                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                        }
                                        else
                                        {
                                            score = dna[base_2];
                                            score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                            score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                        }
                                    }
                                    else
                                    {
                                        score = dna[base_6];
                                        score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                        score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                    }
                                }
                                else        //2
                                {
                                    if ( m_b_situation[miansan] + m_b_situation[huoer] + m_b_situation[huosan] >= 2 )
                                    {
                                        score = dna[base_2];
                                    }
                                    else
                                    {
                                        if ( m_b_situation[miansan] + m_b_situation[huoer] + m_b_situation[huosan] == 1 )
                                        {
                                            score = dna[base_3];
                                        }
                                        else
                                        {
                                            score = dna[base_6];
                                        }
                                    }
                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                }
                            }

                        }
                        else
                        {
                            if(m_w_situation[huosan]>=1) //7
                            {
                                score = dna[base_7];
                                if(m_w_situation[huoer]>=3&&m_w_detail[huoer][huoer]>0)
                                {
                                    score -= m_w_situation[huoer] *40;
                                }
                                else
                                {
                                    score -= m_w_situation[huoer] *20;
                                }
                                if(m_w_situation[huosan]>=2&&(m_w_detail[huoer][huosan]+m_w_detail[huoer][huoer])>0)
                                {
                                    score -= m_w_situation[huosan] *50;
                                }
                                else
                                {
                                    score -= m_w_situation[huosan] *40;
                                }
                                score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                            }
                            else
                            {
                                if(m_w_situation[huoer] >= 2 && (m_w_detail[huoer][huoer] + m_w_detail[huoer][miansan] + m_w_detail[huoer][sisi] + m_w_vir_detail[huoer][huoer] + m_w_vir_detail[huoer][miansan])>0)  //7
                                {
                                    score = dna[base_7];
                                    score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                    score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                }
                                else
                                {
                                    if( m_w_situation[huoer] >=2 ) //6
                                    {
                                        score = dna[base_6];
                                        score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                        score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                    }
                                    else
                                    {
                                        if(m_w_situation[huoer]>=1 &&( m_w_detail[huoer][sisi] +m_w_detail[huoer][miansan] + m_w_vir_detail[huoer][miansan])> 0) //6
                                        {
                                            score = dna[base_7];
                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                        }
                                        else
                                        {
                                            if(m_w_situation[miansan] >=2 &&m_w_detail[miansan][miansan] >0)  //6
                                            {
                                                score = dna[base_7];
                                                score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                            }

                                            else
                                            {
                                                if ( m_w_situation[huoer] >= 1 )
                                                {
                                                    score = dna[base_6];
                                                    score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                    score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                                }
                                                else
                                                {
                                                    if ( m_w_situation[miansan] >= 1 && m_w_situation[huoer] == 0 )
                                                    {
                                                        if ( ( m_b_situation[miansan] >= m_w_situation[miansan] && m_b_situation[huoer] + m_b_situation[huosan] > 2 ) && ( m_b_detail[huoer][miansi] + m_b_detail[huosan][miansi] + m_b_detail[huoer][huoer] + m_b_vir_detail[huoer][huoer]> 0 ) )
                                                        {
                                                            score = dna[base_2];
                                                            score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                            score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                        }
                                                        else
                                                        {
                                                            if ( ( m_b_situation[miansan] >= m_w_situation[miansan] && m_b_situation[huoer] + m_b_situation[huosan] == 2 ) && ( m_b_detail[huoer][miansi] + m_b_detail[huosan][miansi] + m_b_detail[huoer][huoer] + m_b_vir_detail[huoer][huoer]> 0 ) )
                                                            {
                                                                if ( m_b_situation[huosan] > 0 )
                                                                {
                                                                    score = dna[base_2];
                                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                                }
                                                                else
                                                                {
                                                                    score = dna[base_3];
                                                                    score += m_b_situation[huosan] * dna[huosan_big] + m_b_situation[huoer] * dna[huoer_big] + m_b_situation[miansan] * dna[miansan_big] +m_b_cross * dna[cross_big];
                                                                    score -= m_w_situation[huosan] * dna[huosan_lit] + m_w_situation[huoer] * dna[huoer_lit] + m_w_situation[miansan] * dna[miansan_lit] +m_w_cross * dna[cross_lit];
                                                                }
                                                            }
                                                            else
                                                            {
                                                                score = dna[base_6];
                                                                score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                                score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if ( m_w_situation[miansan] > 0 )
                                                        {
                                                            score = dna[base_6];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];

                                                        }


                                                        else    // 3
                                                        {
                                                            score = dna[base_5];
                                                            score += m_b_situation[huosan] * dna[huosan_lit] + m_b_situation[huoer] * dna[huoer_lit] + m_b_situation[miansan] * dna[miansan_lit] +m_b_cross * dna[cross_lit];
                                                            score -= m_w_situation[huosan] * dna[huosan_big] + m_w_situation[huoer] * dna[huoer_big] + m_w_situation[miansan] * dna[miansan_big] +m_w_cross * dna[cross_big];
                                                        }
                                                    }
                                                }
                                            }

                                        }
                                    }
                                }
                            }
                        }

                        for(i=1;i<=BOARD_SIZE;i++)
                            for(j=1;j<=BOARD_SIZE;j++)
                            {
                                if ( board[i][j] == ourOrder )
                                {
                                    score += g_board_base_score[i-1][j-1];
                                }
                                if ( board[i][j] == 2 )
                                {
                                    score -= g_board_base_score[i-1][j-1];
                                }
                            }
                }
        }


        score += MAXINT/2;
    }

    if (ourOrder == BLACK)
    {
        score += b_mean_point - w_mean_point;
    }else
    {
        score += w_mean_point - b_mean_point;
    }

    end = clock();
    m_time_evalution += (double)(end-beg)/CLOCKS_PER_SEC;

    return (score/10);
}


// ======================= situation.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


void CEvaluation::set_situation(char board[][GRID_NUM])
{
    //ʼ
    int d[4];

    memset(m_w_detail,0,sizeof(m_w_detail));
    memset(m_b_detail,0,sizeof(m_b_detail));
    memset(m_w_situation,0,sizeof(m_w_situation));
    memset(m_b_situation,0,sizeof(m_b_situation));
    memset(m_visited_direction,-1,sizeof(m_visited_direction));

    memset(m_b_vir_detail,0,sizeof(m_b_vir_detail));
    memset(m_w_vir_detail,0,sizeof(m_w_vir_detail));
    memset(m_visited_virtual_direction,-1,sizeof(m_visited_virtual_direction));

    b_mean_point = w_mean_point = 0;

    m_w_cross = m_b_cross = 0;

    //ÿ
    int bd[4],wd[4];
    for (int x = 1 ; x < GRID_NUM - 1 ; x++)
    {
        for (int y = 1 ; y < GRID_NUM - 1 ; y++)
        {
            if (board[x][y] != NOSTONE) 
            {
                // λ [10/28/2011 lang]
                //  [10/28/2011 lang]
                //·
                d[0] = m_visited_direction[x][y][DUD];
                if (d[0] < 0)
                {
                    set_situation_for_one_direction(x,y,1,0,DUD,board);
                    d[0] = m_visited_direction[x][y][DUD];
                }

                //ҷ
                d[1] = m_visited_direction[x][y][DLR];
                if (d[1] < 0)
                {
                    set_situation_for_one_direction(x,y,0,1,DLR,board);
                    d[1] = m_visited_direction[x][y][DLR];
                }

                //Ϸ
                d[2] = m_visited_direction[x][y][DRU];
                if (d[2] < 0)
                {
                    set_situation_for_one_direction(x,y,-1,1,DRU,board);
                    d[2] = m_visited_direction[x][y][DRU];
                }

                //·
                d[3] = m_visited_direction[x][y][DRD];
                if (d[3] < 0)
                {
                    set_situation_for_one_direction(x,y,1,1,DRD,board);
                    d[3] = m_visited_direction[x][y][DRD];
                }

                if (board[x][y] == BLACK)
                {
                    for (int i = 0 ; i < 4 ; i++)
                    {
                        for (int j = i + 1 ; j < 4 ; j++)
                        {
                            if (d[i] >=0 && d[j] >= 0 && d[i] < wuxing && d[j] < wuxing && (d[i]!= sisi || d[j]!=sisi))
                            {
                                m_b_detail[d[j]][d[i]]++;
                                if (i != j)
                                    m_b_detail[d[i]][d[j]]++;

                                m_b_cross++;
                            }
                        }
                    }
                }else
                {
                    for (int i = 0 ; i < 4 ; i++)
                    {
                        for (int j = i + 1 ; j < 4 ; j++)
                        {
                            if (d[i] >=0 && d[j] >= 0 && d[i] < wuxing && d[j] < wuxing && (d[i]!= sisi || d[j]!=sisi))
                            {
                                m_w_detail[d[j]][d[i]]++;
                                if (i != j)
                                    m_w_detail[d[i]][d[j]]++;

                                m_w_cross++;
                            }
                        }
                    }
                }
            }else
            {
                // λ [10/28/2011 lang]
                // 齻 [10/28/2011 lang]
                for (int di = 0 ; di < 4 ; di++)
                {
                    bd[di] = -1;
                    wd[di] = -1;
                    if (m_visited_virtual_direction[x][y][di] >=black_fix)
                    {
                        bd[di] = m_visited_virtual_direction[x][y][di];
                    }else if (m_visited_virtual_direction[x][y][di] >=white_fix)
                    {
                        wd[di] = m_visited_virtual_direction[x][y][di];
                    }
                }
                for (int i = 0 ; i < 4 ; i++)
                {
                    for (int j = i + 1 ; j < 4 ; j++)
                    {
                        if (bd[i] >=0 && bd[j] >= 0 )
                        {
                            m_b_vir_detail[bd[j] - black_fix][bd[i] - black_fix]++;
                            if (i != j)
                                m_b_vir_detail[bd[i] - black_fix][bd[j] - black_fix]++;
                        }

                        if (wd[i] >=0 && wd[j] >= 0 )
                        {
                            m_w_vir_detail[wd[j] - white_fix][wd[i] - white_fix]++;
                            if (i != j)
                                m_w_vir_detail[wd[i]- white_fix][wd[j] - white_fix]++;
                        }
                    }
                }
            }
        }
    }

    return ;
}

void CEvaluation::set_situation_for_one_direction(int x, int y ,short countx,short county,int dir, char board[][GRID_NUM])
{
    char currColor = board[x][y];
    int i = x,j = y;
    int colorN = 1,noneN = 0;
    int longContinue = 1;
    int longNone = 0;
    int rLongest = 0;
    int lLongest = 0;

    int tempLong = 1;
    int mid = 0;
    int rI,rJ,lI,lJ;
    //int total = 0;
    int rEnd = 0,lEnd = 0;

    pos_t posList[20] ;
    pos_t posRList[20] ;
    pos_t posLList[20] ;
    memset(posList, 0, sizeof(posList));
    memset(posRList, 0, sizeof(posRList));
    memset(posLList, 0, sizeof(posLList));
    posList[0].x = 1;
    posList[1].x = x;
    posList[1].y = y;
    posLList[0].x = 0;
    posRList[0].x = 0;
    int count= 0;

    //һɫ
    int rSideColor = board[x-countx][y-county],lSideColor = board[x+countx][y+county];

    while ( rEnd < 5 &&  colorN  < 7 && noneN < 4)
    {
        i += countx;
        j += county;

        if (currColor == board[i][j])
        {
            //ǰɫɫͬ
            if (noneN + rEnd >3)
            {
                break;
            }
            if (longNone < rEnd)
            {
                longNone = rEnd;
            }
            noneN += rEnd;
            rEnd = 0;
            colorN++;

            tempLong++;

            //ʹĵ
            posList[++posList[0].x].x = i;
            posList[posList[0].x].y = j;

            rSideColor = board[i-countx][j-county];

        } else if (board[i][j] == NOSTONE)
        {
            //ǰ
            count++;
            if (rLongest == 0)
                rLongest = tempLong;

            rLongest = tempLong;

            if (tempLong > longContinue)
            {
                longContinue = tempLong;
            }
            if (mid == 0)
                mid = tempLong;
            tempLong = 0;
            rEnd++;

            //ʹĵ
            posRList[++posRList[0].x].x = i;
            posRList[posRList[0].x].y = j;
        } else 
        {
            //
            if (rLongest == 0)
                rLongest = tempLong;

            if (tempLong > longContinue)
            {
                longContinue = tempLong;
            }
            break;
        }
    }
    rI = i;
    rJ = j;

    i = x;
    j = y;
    tempLong = mid;
    while (lEnd < 5 && colorN  < 7)
    {
        i -= countx;
        j -= county;
        
        if (currColor == board[i][j])
        {
            //ǰɫɫͬ
            if (noneN + lEnd >3)
            {
                break;
            }
            if (longNone < lEnd)
            {
                longNone = lEnd;
            }
            noneN += lEnd;
            lEnd = 0;
            colorN++;

            tempLong++;

            //ʹĵ
            posList[++posList[0].x].x = i;
            posList[posList[0].x].y = j;

            lSideColor = board[i+countx][j+county];

        } else if (board[i][j] == NOSTONE)
        {
            //ǰ
            if (lLongest == 0)
                lLongest = tempLong;
            lLongest = tempLong;

            if (tempLong > longContinue)
            {
                longContinue = tempLong;
            }
            if (count == 1)
            {
                //ǰֵ
                rLongest = tempLong;
                count = 0;
            }
            tempLong = 0;
            lEnd++;

            //ʹĵ
            posLList[++posLList[0].x].x = i;
            posLList[posLList[0].x].y = j;
        } else 
        {
            //
            if (lLongest == 0)
                lLongest = tempLong;

            if (tempLong > longContinue)
            {
                longContinue = tempLong;
            }
            break;
        }
    }
    lI = i;
    lJ = j;

    for(int i = 0 ; i <= posList[0].x ; i++)
    {
        m_visited_direction[posList[i].x][posList[i].y][dir] = wuxing;
    }

    if (rEnd + lEnd + colorN + noneN < 6 && colorN != 4)
    {
        //һеܺСγΣ
        return ;
    }else if (rEnd + lEnd + colorN + noneN > 5)
    {
        if (currColor == BLACK)
        {
            int last = posList[0].x - (rEnd < 2 ? 2-rEnd : 0) - (lEnd < 2 ? 2 - lEnd: 0);

            if (rSideColor == NOSTONE && rEnd == 0 || lSideColor == NOSTONE && lEnd == 0)
            {
                last++;
            }

            b_mean_point += last < 0 ? 0 : last;
        }else
        {
            int last = posList[0].x - (rEnd < 2 ? 2-rEnd : 0) - (lEnd < 2 ? 2 - lEnd: 0);

            if (rSideColor == NOSTONE && rEnd == 0 || lSideColor == NOSTONE && lEnd == 0)
            {
                last++;
            }

            w_mean_point += last < 0 ? 0 : last;
        }
    }

    switch (colorN)
    {
    case 1:        //color
        break;
    case 2:        //color
        if (rEnd > 1 && lEnd > 1 && rEnd + lEnd + noneN >= 6 && noneN < 3)
        {
            //
            if (currColor == BLACK)
            {
                m_b_situation[huoer]++;

                for (int i = 1 ; i <= posRList[0].x - rEnd + 2 ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huoer + black_fix;
                }
                for (int i = 1 ; i <= posLList[0].x - lEnd + 2 ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huoer + black_fix ;
                }
            }else
            {
                m_w_situation[huoer]++;

                for (int i = 1 ; i <= posRList[0].x - 2 ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huoer + white_fix;
                }
                for (int i = 1 ; i <= posLList[0].x - 2 ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huoer + white_fix;
                }
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = huoer;
            }

        } else
        {
            //
            //if (currColor == BLACK)
            //{
            //    situation_b[huoer]++;
            //}else
            //{
            //    situation_w[huoer]++;
            //}
            //for (int i = 0 ; i <= posList[0].x ; i++)
            //{
            //    visitedDir[posList[i].x][posList[i].y][dir] = huoer;
            //}
        }
        break;
    case 3:        //color
        if (rEnd > 1 && lEnd > 1 && rEnd + lEnd + noneN >= 5 && noneN < 2)
        {
            //
            if (currColor == BLACK)
            {
                m_b_situation[huosan]++;

                for (int i = 1 ; i <= posRList[0].x - 2 ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huosan + black_fix;
                }
                for (int i = 1 ; i <= posLList[0].x - 2 ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huosan + black_fix ;
                }
            }else
            {
                m_w_situation[huosan]++;

                for (int i = 1 ; i <= posRList[0].x - 2 ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huosan + white_fix;
                }
                for (int i = 1 ; i <= posLList[0].x - 2 ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huosan + white_fix ;
                }
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = huosan;
            }

        }else if ((rEnd == 1 && noneN < 3 && noneN + lEnd > 2)||( lEnd == 1 && noneN < 3 && noneN + rEnd > 2))
        {
            //൱ڻ 0***000
            if (currColor == BLACK)
            {
                m_b_situation[huoer]++;

                for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huoer + black_fix;
                }
                for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huoer + black_fix ;
                }
            }else
            {
                m_w_situation[huoer]++;

                for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huoer + white_fix;
                }
                for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huoer + white_fix ;
                }
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = huoer;
            }
            //
            if (currColor == BLACK)
            {
                m_b_situation[miansan]++;

                for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = miansan + black_fix;
                }
                for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = miansan + black_fix ;
                }
            }else
            {
                m_w_situation[miansan]++;

                for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = miansan + white_fix;
                }
                for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = miansan + white_fix ;
                }
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = miansan;
            }
        } else
        {
            //кл,
            if (( longNone == 1 && ( lLongest == 1 && rLongest == 1 && (rEnd > 2 || lEnd > 2)|| rEnd > 3 && rLongest == 2 || lEnd > 3 && lLongest ==2)  ) || 
                ( longNone == 2 && (lLongest == 1 && rLongest == 1 && (rEnd > 2 || lEnd > 2) || rEnd > 2 && rLongest == 2 || lEnd > 2 && lLongest ==2)  ) ||
                ( longNone == 3 && (rLongest == 2 && rEnd > 1 || lLongest == 2 && lEnd > 1)))
            {
                if (currColor == BLACK)
                {
                    m_b_situation[huoer]++;

                    for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                    {
                        m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huoer + black_fix;
                    }
                    for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                    {
                        m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huoer + black_fix ;
                    }

                }else
                {
                    m_w_situation[huoer]++;

                    for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                    {
                        m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = huoer + white_fix;
                    }
                    for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                    {
                        m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = huoer + white_fix ;
                    }
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = huoer;
                }
            }
            //
            if (currColor == BLACK)
            {
                m_b_situation[miansan]++;

                for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = miansan + black_fix;
                }
                for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = miansan + black_fix ;
                }
            }else
            {
                m_w_situation[miansan]++;

                for (int i = 1 ; i <= posRList[0].x + ( rEnd > 2 ? 2 - rEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posRList[i].x][posRList[i].y][dir] = miansan + white_fix;
                }
                for (int i = 1 ; i <= posLList[0].x + ( lEnd > 2 ? 2 - lEnd: 0 ) ; i++)
                {
                    m_visited_virtual_direction[posLList[i].x][posLList[i].y][dir] = miansan + white_fix ;
                }
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = miansan;
            }

        }
        break;
    case 4:        //color
        if (rEnd + lEnd + colorN + noneN < 6)
        {
            //ģ
            if (currColor == BLACK)
            {
                m_b_situation[sisi]++;
            }else
            {
                m_w_situation[sisi]++;
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = sisi;
            }
            break;

        }
        if (rEnd > 1 && lEnd > 1 && noneN == 0)
        {
            //
            if (currColor == BLACK)
            {
                m_b_situation[huosi]++;
            }else
            {
                m_w_situation[huosi]++;
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = huosi;
            }
        }else if (noneN < 3)
        {
            //
            if (currColor == BLACK)
            {
                m_b_situation[miansi]++;
            }else
            {
                m_w_situation[miansi]++;
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = miansi;
            }
        }else if (noneN == 3)
        {
            if (longNone == 1)
            {
                if (rEnd > 1 && lEnd > 1)
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[huosan]++;
                    }else
                    {
                        m_w_situation[huosan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = huosan;
                    }
                }else 
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[miansan]++;
                    }else
                    {
                        m_w_situation[miansan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = miansan;
                    }
                }
            }else if (longNone == 2)
            {
                //ӿ3
                if (rLongest == 2 && rEnd < 2 || lLongest == 2 && lEnd < 2)
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[miansan]++;
                    }else
                    {
                        m_w_situation[miansan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = miansan;
                    }

                }else 
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[huosan]++;
                    }else
                    {
                        m_w_situation[huosan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = huosan;
                    }
                }

            }else
            {
                if (rEnd < 2 && rLongest == 3 || lEnd < 2 && lLongest == 3)
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[huosan]++;
                    }else
                    {
                        m_w_situation[huosan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = huosan;
                    }
                }else
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[huosan]++;
                    }else
                    {
                        m_w_situation[huosan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = huosan;
                    }
                }
            }
        }
        break;
    case 5:        //color
        switch (longContinue)
        {
        case 2:
            //
            if (currColor == BLACK)
            {
                m_b_situation[mianwu]++;
            }else
            {
                m_w_situation[mianwu]++;
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = mianwu;
            }

            break;
        case 3:
            if (longNone == 3)
            {
                //
                if (lEnd < 2 && rEnd < 2)
                {
                    //
                    if (currColor == BLACK)
                    {
                        m_b_situation[miansan]++;
                    }else
                    {
                        m_w_situation[miansan]++;
                    }
                    for (int i = 0 ; i <= posList[0].x ; i++)
                    {
                        m_visited_direction[posList[i].x][posList[i].y][dir] = miansan;
                    }
                }else 
                {
                    if (lEnd > 1 && lLongest == 3 || rEnd > 1 && rLongest ==3)
                    {
                        //
                        if (currColor == BLACK)
                        {
                            m_b_situation[huosan]++;
                        }else
                        {
                            m_w_situation[huosan]++;
                        }
                        for (int i = 0 ; i <= posList[0].x ; i++)
                        {
                            m_visited_direction[posList[i].x][posList[i].y][dir] = huosan;
                        }
                    }
                }
            }else if (rLongest == 3 && lLongest == 3)
            {
                //
                if (currColor == BLACK)
                {
                    m_b_situation[huowu]++;
                }else
                {
                    m_w_situation[huowu]++;
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = huowu;
                }
            }else
            {
                //
                if (currColor == BLACK)
                {
                    m_b_situation[mianwu]++;
                }else
                {
                    m_w_situation[mianwu]++;
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = mianwu;
                }
            }
            //if (rEnd > 0 && lEnd > 0 && noneN == 2)
            //{
            //    //
            //    if (currColor == BLACK)
            //    {
            //        situation_b[huowu]++;
            //    }else
            //    {
            //        situation_w[huowu]++;
            //    }
            //    for (int i = 0 ; i <= posList[0].x ; i++)
            //    {
            //        visitedDir[posList[i].x][posList[i].y][dir] = huowu;
            //    }
            //}else
            //{
            //    //
            //    if (currColor == BLACK)
            //    {
            //        situation_b[mianwu]++;
            //    }else
            //    {
            //        situation_w[mianwu]++;
            //    }
            //    for (int i = 0 ; i <= posList[0].x ; i++)
            //    {
            //        visitedDir[posList[i].x][posList[i].y][dir] = mianwu;
            //    }
            //}
            break;
        case 4:
            if (rEnd < 2 && rLongest == 4 || lEnd < 2 && lLongest == 4)
            {
                //
                if (currColor == BLACK)
                {
                    m_b_situation[mianwu]++;
                }else
                {
                    m_w_situation[mianwu]++;
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = mianwu;
                }
            } else
            {
                //
                if (currColor == BLACK)
                {
                    m_b_situation[huowu]++;
                }else
                {
                    m_w_situation[huowu]++;
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = huowu;
                }
            }
            //if (rEnd > 0 && lEnd > 0)
            //{
            //    //
            //    if (currColor == BLACK)
            //    {
            //        situation_b[huowu]++;
            //    }else
            //    {
            //        situation_w[huowu]++;
            //    }
            //    for (int i = 0 ; i <= posList[0].x ; i++)
            //    {
            //        visitedDir[posList[i].x][posList[i].y][dir] = huowu;
            //    }
            //}else if (rEnd + lEnd == 1)
            //{
            //    //
            //    while (board[i][j] != currColor)
            //    {
            //        //
            //        i += countx;
            //        j += county;
            //    }
            //    if (board[i+countx][j+county] == board[i-countx][j-county])
            //    {
            //        //
            //        if (currColor == BLACK)
            //        {
            //            situation_b[mianwu]++;
            //        }else
            //        {
            //            situation_w[mianwu]++;
            //        }
            //        for (int i = 0 ; i <= posList[0].x ; i++)
            //        {
            //            visitedDir[posList[i].x][posList[i].y][dir] = mianwu;
            //        }
            //    }else 
            //    {
            //        //
            //        if (currColor == BLACK)
            //        {
            //            situation_b[huowu]++;
            //        }else
            //        {
            //            situation_w[huowu]++;
            //        }
            //        for (int i = 0 ; i <= posList[0].x ; i++)
            //        {
            //            visitedDir[posList[i].x][posList[i].y][dir] = huowu;
            //        }
            //    }
            //}else
            //{
            //    //
            //    if (currColor == BLACK)
            //    {
            //        situation_b[mianwu]++;
            //    }else
            //    {
            //        situation_w[mianwu]++;
            //    }
            //    for (int i = 0 ; i <= posList[0].x ; i++)
            //    {
            //        visitedDir[posList[i].x][posList[i].y][dir] = mianwu;
            //    }
            //}
            break;
        case 5:
            if (rEnd > 0 && lEnd > 0)
            {
                //
                if (currColor == BLACK)
                {
                    m_b_situation[huowu]++;
                }else
                {
                    m_w_situation[huowu]++;
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = huowu;
                }
            }else
            {
                //
                if (currColor == BLACK)
                {
                    m_b_situation[mianwu]++;
                }else
                {
                    m_w_situation[mianwu]++;
                }
                for (int i = 0 ; i <= posList[0].x ; i++)
                {
                    m_visited_direction[posList[i].x][posList[i].y][dir] = mianwu;
                }
            }
            break;
        default :
            //
            if (currColor == BLACK)
            {
                m_b_situation[mianwu]++;
            }else
            {
                m_w_situation[mianwu]++;
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = mianwu;
            }
            break;
        }
        break;
    case 6:        //color
        if (noneN == 0)
        {
            //Ѿ
            if (currColor == BLACK)
            {
                m_b_situation[liu]++;
            }else
            {
                m_w_situation[liu]++;
            }
            for (int i = 0 ; i <= posList[0].x ; i++)
            {
                m_visited_direction[posList[i].x][posList[i].y][dir] = liu;
            }
        }
        break;
    case 7:        //color
        switch (noneN)
        {
        case 0:        //noneN
            break;
        case 1:        //noneN
            break;
        case 2:        //noneN
            break;
        case 3:        //noneN
            break;
        default :        //noneN
            break;
        }
        break;
    case 8:        //color
        switch (noneN)
        {
        case 0:        //noneN
            break;
        case 1:        //noneN
            break;
        case 2:        //noneN
            break;
        case 3:        //noneN
            break;
        default :        //noneN
            break;
        }
        break;
    default :    //color
        switch (noneN)
        {
        case 0:        //noneN
            break;
        case 1:
            break;
        case 2:        //noneN
            break;
        case 3:        //noneN
            break;
        default :        //noneN
            break;
        }
        break;
    }

    return ;
}


// ======================= tools.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


char g_log_file_name[] = "engine.log";

void init_board(char board[][GRID_NUM])
{
    // Board init. [10/28/2011 lang]
    for(int i = 0; i< GRID_NUM ; i++)
    {
        board[i][0] = board[0][i] = board[i][GRID_NUM-1] = board[GRID_NUM-1][i] = BORDER;
    }
    for(int i = 1 ; i< GRID_NUM-1 ; i++)
    {
        for(int j = 1 ; j < GRID_NUM-1 ; j++)
        {
            board[i][j] = NOSTONE;
        }
    }
}

void make_move(char board[][GRID_NUM], move_t* move, char color) {
    board[move->positions[0].x][move->positions[0].y] = color;
    board[move->positions[1].x][move->positions[1].y] = color;
}

void unmake_move(char board[][GRID_NUM], move_t* move)
{
    board[move->positions[0].x][move->positions[0].y] = NOSTONE;
    board[move->positions[1].x][move->positions[1].y] = NOSTONE;
}

bool is_win_by_premove(char board[][GRID_NUM], move_t *preMove)
{
    int count = 0,i,j,n,m;

    // The first point.
    n = i = preMove->positions[0].x;
    m = j = preMove->positions[0].y;
    // Horizon direction.
    count = 0;
    if (board[n][m] == BORDER
            || board[n][m] == NOSTONE)
    {
        return false;
    }
    while ( board[i][j] == board[n][m])
    {
        i++;
        count++;
    }
    i = n-1;
    while ( board[i][j] == board[n][m])
    {
        i--;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // Left up direction.
    count = 0;
    i = n;j = m;
    while ( board[i][j] == board[n][m])
    {
        i++;
        j++;
        count++;
    }
    i = n-1;
    j = m-1;
    while ( board[i][j] == board[n][m])
    {
        i--;
        j--;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // Vertical direction.
    count = 0;
    i = n;j = m;
    while ( board[i][j] == board[n][m])
    {
        j++;
        count++;
    }
    j = m-1;
    while (board[i][j] == board[n][m])
    {
        j--;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // Down left direction.
    count = 0;
    i = n;j = m;
    while ( board[i][j] == board[n][m])
    {
        i++;
        j--;
        count++;
    }
    i = n-1;
    j = m+1;
    while ( board[i][j] == board[n][m])
    {
        i--;
        j++;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // The second point.
    n = i = preMove->positions[1].x;
    m = j = preMove->positions[1].y;
    if (board[n][m] == BORDER
            || board[n][m] == NOSTONE)
    {
        return false;
    }
    // Horizon direction.
    count = 0;
    while ( board[i][j] == board[n][m])
    {
        i++;
        count++;
    }
    i = n-1;
    while (board[i][j] == board[n][m])
    {
        i--;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // Up left direction.
    count = 0;
    i = n;j = m;
    while ( board[i][j] == board[n][m])
    {
        i++;
        j++;
        count++;
    }
    i = n-1;
    j = m-1;
    while ( board[i][j] == board[n][m])
    {
        i--;
        j--;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // Vertical direction.
    count = 0;
    i = n;j = m;
    while ( board[i][j] == board[n][m])
    {
        j++;
        count++;
    }
    j = m-1;
    while ( board[i][j] == board[n][m])
    {
        j--;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    // Down left direction.
    count = 0;
    i = n;j = m;
    while ( board[i][j] == board[n][m])
    {
        i++;
        j--;
        count++;
    }
    i = n-1;
    j = m+1;
    while ( board[i][j] == board[n][m])
    {
        i--;
        j++;
        count++;
    }
    if (count >= 6)
    {
        return true;
    }

    return false;
}

int get_msg(char* buf, int maxLen) {
    if (buf == NULL)
        return -1;
    int len;
    char c;
    for (len = 0; len < maxLen; len++) {
        c = getchar();
        if (c == '\n')
            break;
        buf[len] = c;
    }
    buf[len] = 0;
    return len;
}

int log_to_file(char* msg) 
{
    FILE* file = fopen(g_log_file_name, "a");
    if (file == NULL)
    {
        printf("Error: Can't open log file - %s\n", g_log_file_name);
        return -1;
    }
    time_t tm = time(NULL);
    char* ptr;
    ptr = ctime(&tm);
    ptr[strlen(ptr) - 1] = 0;
    fprintf(file, "[%s] - %s\n", ptr, msg);
    fclose(file);

    return 0;
}

int move2msg(move_t* move, char* msg)
{
    if (move->positions[0].x == move->positions[1].x
            && move->positions[0].y == move->positions[1].y)
    {
        msg[0] = 'S' - move->positions[0].x + 1;
        msg[1] = move->positions[0].y + 'A' - 1;
        msg[2] = 0;
        return 1;
    } 
    else
    {
        msg[1] = 'S' - move->positions[0].x + 1;
        msg[0] = move->positions[0].y + 'A' - 1;
        msg[3] = 'S' - move->positions[1].x + 1;
        msg[2] = move->positions[1].y + 'A' - 1;

        //msg[0] = move->positions[0].x + 'A' - 1;
        //msg[1] = move->positions[0].y + 'A' - 1;
        //msg[2] = move->positions[1].x + 'A' - 1;
        //msg[3] = move->positions[1].y + 'A' - 1;
        msg[4] = 0;
    }
    return 2;
}

int msg2move(char* msg, move_t* move)
{
    if (msg[2] == 0)
    {
        move->positions[0].x = move->positions[1].x = 'S' - msg[1] + 1;
        move->positions[0].y = move->positions[1].y = msg[0] - 'A' + 1;
        move->score = 0;
        return 1;
    } 
    else
    {
        move->positions[0].x = 'S' - msg[1] + 1;
        move->positions[0].y = msg[0] - 'A' + 1;
        move->positions[1].x = 'S' - msg[3] + 1;
        move->positions[1].y = msg[2] - 'A' + 1;

        //move->positions[0].x = msg[0] - 'A' + 1;
        //move->positions[0].y = msg[1] - 'A' + 1;
        //move->positions[1].x = msg[2] - 'A' + 1;
        //move->positions[1].y = msg[3] - 'A' + 1;
        move->score = 0;
    }
    return 2;
}

void print_board(char board[][GRID_NUM], move_t* preMove)
{
    printf("  ");
    for (int i = 1 ; i < GRID_NUM - 1 ; i++)
    {
        printf("%2c", 'A' + i - 1);
    }
    printf("\n");
    int x,y;
    for (int i = 1 ; i < GRID_NUM - 1 ; i++)
    {
        printf("%2c", 'T' - i);
        for (int j = 1 ; j < GRID_NUM - 1; j++)
        {
            //x = j;
            //y = GRID_NUM - i - 1;
            x = i;
            y = j;
            if (preMove) {
                if (x == preMove->positions[0].x 
                        && y == preMove->positions[0].y 
                        && board[x][y] != NOSTONE
                        || x == preMove->positions[1].x 
                        && y == preMove->positions[1].y 
                        && board[x][y] != NOSTONE )
                {
                    printf(" X");
                    continue;
                }
            }
            switch (board[x][y])
            {
                case  WHITE:
                    printf(" O");
                    break;
                case BLACK:
                    printf(" *");
                    break;
                case NOSTONE:
                    printf(" -");
                    break;
            }
        }
        printf("%2c", 'T' - i);
        printf("\n");
    }
    printf("  ");
    for (int i = 1 ; i < GRID_NUM - 1 ; i++)
    {
        printf("%2c", 'A' + i - 1);
    }
    printf("\n");
}

void print_score(move_one_t *moveList,int n)
{
    int board[GRID_NUM][GRID_NUM];
    memset(board, 0, sizeof(board));
    for (int i = 0 ; i < n ; i++)
    {
        board[moveList[i].x][moveList[i].y] = moveList[i].score;
    }

    printf("  ");
    for (int i = 1 ; i < GRID_NUM - 1 ; i++)
    {
        printf("%4d",i);
    }
    printf("\n");
    int score = 0,x,y;
    for (int i = 1 ; i < GRID_NUM - 1 ; i++)
    {
        printf("%2d", i);
        for (int j = 1 ; j < GRID_NUM - 1; j++)
        {
            //x = j;
            //y = GRID_NUM - i - 1;
            x = i;
            y = j;
            score = board[x][y];
            if (score == 0)
            {
                printf("   -");
            } 
            else
            {
                printf("%4d", score);
            }
        }
        printf("\n");
    }
}


// ======================= pattern.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


#define l_2_r       2
#define r_2_l       3
#define u_2_d       0
#define d_2_u       1
#define lu_2_rd     4
#define ld_2_ru     6
#define ru_2_ld     5
#define rd_2_lu     7

#define Size        100

/* Directions */
pos_t transformation2[8] = {
  { 1,  0}, /* from left to right */

  {-1,  0}, /* from right to left */

  { 0,  1}, /* from up to down */

  { 0, -1}, /* from down to up */

  { 1,  1}, /* from left up corner to right down corner*/

  { 1, -1}, /* from left down corner to right up corner*/

  {-1,  1}, /* from right up corner to left down corner*/

  {-1, -1}  /* from right down corner to left up corner*/
};

struct sim_type{
    int x, y;
    int dfa;
}simple[100];

CDFA::CDFA()
{
    memset(this, 0, sizeof(CDFA));
}

int CDFA::change(int Color)
{
    if ( Color == 1 )
        return Color+my_color;
    if ( Color == 2 )
        return Color-my_color;
    return Color;
}

int CDFA::check(move_t bestMove[], move_t now)
{
    int i;
    for (i=0; i<count; i++){
        if (now.positions[0].x == bestMove[i].positions[0].x &&
            now.positions[1].x == bestMove[i].positions[1].x &&
            now.positions[0].y == bestMove[i].positions[0].y &&
            now.positions[1].y == bestMove[i].positions[1].y)
            return 0;
        if (now.positions[0].x == bestMove[i].positions[1].x &&
            now.positions[1].x == bestMove[i].positions[0].x &&
            now.positions[0].y == bestMove[i].positions[1].y &&
            now.positions[1].y == bestMove[i].positions[0].y)
            return 0;
    }
    return 1;
}

void CDFA::new_match2(pos_t point, dfa_t *pdfa, move_t bestMove[], int direction)
{
    int p=0, j, step;
    for (step=0; step<2*pdfa->last_state; step++){
        p = pdfa->states[p].next[change(m_board[point.x][point.y])];
        if (p == pdfa->last_state){
            for (j=0; j<pdfa->last_index; j++){

                if ( pdfa->indexes[j].mode == 1 ){

                    bestMove[count].positions[0].x = simple[sim_c-1].x;
                    bestMove[count].positions[0].y = simple[sim_c-1].y;
                    bestMove[count].positions[1].x = pdfa->indexes[j].offset[0] * (-transformation2[direction].x) + point.x;
                    bestMove[count].positions[1].y = pdfa->indexes[j].offset[0] * (-transformation2[direction].y) + point.y;
                    if (check(bestMove, bestMove[count]))
                        count++;
                }
            }
            p = 0;
        }
        point.x += transformation2[direction].x;
        point.y += transformation2[direction].y;
        if (point.x > BOARD_SIZE || point.y > BOARD_SIZE || point.x < 1 || point.y < 1)
            return;
    }
}

void CDFA::new_match(pos_t point, move_t bestMove[], int ori_direction)
{
    int i, direction;
    pos_t temp;
    for (i=0; i<m_dfa_index; i++){
        for (direction=0; direction<8; direction++){
            if (direction == ori_direction ||
                (transformation2[direction].x == -transformation2[ori_direction].x &&
                transformation2[direction].y == -transformation2[ori_direction].y))
                continue;
            temp.x = point.x - (transformation2[direction].x * (m_dfa_array[i].last_state-1));
            temp.y = point.y - (transformation2[direction].y * (m_dfa_array[i].last_state-1));
            if (temp.x > BOARD_SIZE)
                temp.x = BOARD_SIZE;
            if (temp.y > BOARD_SIZE)
                temp.y = BOARD_SIZE;
            if (temp.x < 1)
                temp.x = 1;
            if (temp.y < 1)
                temp.y = 1;
            new_match2(temp, &m_dfa_array[i], bestMove, direction);
        }
    }
}

void CDFA::addpoint(move_t bestMove[], pos_t point)
{
    int i, j, k;
    for (i=1; i<=BOARD_SIZE; i++){
        for (j=1; j<=BOARD_SIZE; j++){
            if (m_board[i][j] == 0){
                for (k=0; k<8; k++){
                    if (m_board[i+transformation2[k].x][j+transformation2[k].y] == 1 ||
                        m_board[i+transformation2[k].x][j+transformation2[k].y] == 2 ||
                        (i != 1 && i != BOARD_SIZE && j != 1 && j != BOARD_SIZE &&
                        (m_board[i+2*transformation2[k].x][j+2*transformation2[k].y] == 1 ||
                        m_board[i+2*transformation2[k].x][j+2*transformation2[k].y] == 2))){
                              break;
                    }
                }
                if (k < 8){
                    bestMove[count].positions[0].x = point.x;
                    bestMove[count].positions[0].y = point.y;
                    bestMove[count].positions[1].x = i;
                    bestMove[count].positions[1].y = j;
                    if (check(bestMove, bestMove[count]))
                        count++;
                }
            }
        }
    }
}

// Match function.
void CDFA::match2(pos_t point, dfa_t *pdfa, move_t bestMove[], int direction, int dfa_num)
{
    int p=0, j;
    pos_t temp, temp_bestMove;
    while (1){
        p = pdfa->states[p].next[change(m_board[point.x][point.y])];
        if (p == pdfa->last_state){
            for (j=0; j<pdfa->last_index; j++){
                // If it forms two threats, add the move.
                if ( pdfa->indexes[j].mode == 2 ){
                    if (pdfa->indexes[j].offset[0] != pdfa->indexes[j].offset[1]){
                        bestMove[count].positions[0].x = pdfa->indexes[j].offset[0] * (-transformation2[direction].x) + point.x;
                        bestMove[count].positions[0].y = pdfa->indexes[j].offset[0] * (-transformation2[direction].y) + point.y;
                        bestMove[count].positions[1].x = pdfa->indexes[j].offset[1] * (-transformation2[direction].x) + point.x;
                        bestMove[count].positions[1].y = pdfa->indexes[j].offset[1] * (-transformation2[direction].y) + point.y;
                        if (m_board[bestMove[count].positions[0].x][bestMove[count].positions[0].y] != 0 ||
                            m_board[bestMove[count].positions[1].x][bestMove[count].positions[1].y] != 0)
                            //continue;
                            printf("It is wrong!!!!00000000000000000               %d\n",dfa_num);

                        if (check(bestMove, bestMove[count]))
                            count++;
                        }
                    else{
                        temp_bestMove.x = pdfa->indexes[j].offset[0] * (-transformation2[direction].x) + point.x;
                        temp_bestMove.y = pdfa->indexes[j].offset[0] * (-transformation2[direction].y) + point.y;
                        addpoint(bestMove, temp_bestMove);

                    }
                } else{
                    // If it form one threat, record the point.
                    //Moves = (SMove *)realloc( bestMove, count * sizeof(Moves));
                    simple[sim_c].dfa = dfa_num;
                    simple[sim_c].x = pdfa->indexes[j].offset[0] * (-transformation2[direction].x) + point.x;
                    simple[sim_c].y = pdfa->indexes[j].offset[0] * (-transformation2[direction].y) + point.y;

                    for (int i=0; i<sim_c; i++){
                        if (simple[i].x == simple[sim_c].x && simple[i].y == simple[sim_c].y){
                            temp_bestMove.x = simple[sim_c].x;
                            temp_bestMove.y = simple[sim_c].y;
                            addpoint(bestMove, temp_bestMove);
                        }
                    }

                    if (m_board[simple[sim_c].x][simple[sim_c].y] != 0)
                        //continue;
                        printf("It is wrong!!!!1111111111111111111                %d\n",dfa_num);

                    else{
                        sim_c++;
                        temp.x = simple[sim_c-1].x;
                        temp.y = simple[sim_c-1].y;
                        m_board[temp.x][temp.y] = 2-my_color;
                        new_match(temp, bestMove, direction);
                        m_board[temp.x][temp.y] = 0;
                    }

                }
            }
            p = 0;
        }
        point.x += transformation2[direction].x;
        point.y += transformation2[direction].y;
        if (point.x > BOARD_SIZE || point.y > BOARD_SIZE || point.x < 1 || point.y < 1)
            break;
    }
}

// Match function, from the first point in a line.
void CDFA::match(pos_t point, int direction, move_t * bestMove)
{
    int i;
    pos_t temp;
    for (i=0; i<m_dfa_index; i++){
        temp = point;
        // Match according to DFA.
        match2(temp, &m_dfa_array[i], bestMove, direction, i);
    }
    return;
}

int CDFA::pattern_match(char ourColor, move_t bestMove[], char board[][GRID_NUM])
{
    int i, j;
    //int temp_count;
    pos_t point;

    //char temp_board[GRID_NUM][GRID_NUM];

    m_board = board;
    my_color = 0;
    if ( ourColor == 1 )
        my_color = 1;
    //copy_board(m_board, temp_board);

    count = 0;
    sim_c = 0;
    // Match from the four edges of the board.
    for (i=1; i<=GRID_NUM-2; i++){
        // Up side.
        point.x = 1;
        point.y = i;
        match(point, u_2_d, bestMove);
        match(point, lu_2_rd, bestMove);
        match(point, ru_2_ld, bestMove);

        // Left side.
        point.x = i;
        point.y = 1;
        match(point, l_2_r, bestMove);
        match(point, lu_2_rd, bestMove);
        match(point, ld_2_ru, bestMove);

        // Down side.
        point.x = GRID_NUM-2;
        point.y = i;
        match(point, d_2_u, bestMove);
        match(point, ld_2_ru, bestMove);
        match(point, rd_2_lu, bestMove);

        // Right side.
        point.x = i;
        point.y = GRID_NUM-2;
        match(point, r_2_l, bestMove);
        match(point, ru_2_ld, bestMove);
        match(point, rd_2_lu, bestMove);
    }
    // Add moves.
    for (j=0; j<sim_c; j++){
        for (i=j+1; i<sim_c; i++){
            bestMove[count].positions[0].x = simple[j].x;
            bestMove[count].positions[0].y = simple[j].y;
            bestMove[count].positions[1].x = simple[i].x;
            bestMove[count].positions[1].y = simple[i].y;
            if( (bestMove[count].positions[0].x != bestMove[count].positions[1].x ||
                 bestMove[count].positions[0].y != bestMove[count].positions[1].y) &&
                (m_board[bestMove[count].positions[0].x][bestMove[count].positions[0].y] == 0 &&
                 m_board[bestMove[count].positions[1].x][bestMove[count].positions[1].y] == 0)
               ){
                if (check(bestMove, bestMove[count]))
                    count++;
            }
        }
    }
    //kill_dfa();
    //temp_count = count;
    //copy_board(temp_board, m_board);
    return count;
}

// Change the pattern representation.
int CDFA::find(char temp)
{
    if ( strchr("*", temp) )
        return 0;
    if ( strchr("X", temp) )
        return 1;
    if ( strchr("O", temp) )
        return 2;
    if ( strchr("$#+-|", temp) )
        return 3;
    return 0;
}

/*
 * Create the pattern from string.
 * For example:
 * create_dfa(pdfa, "OOX.")
 * gives:
 *
 *           2               2               1              0
 * (1,{}) -------> (2,{}) -------> (3,{}) -------> (4,{}) ------> (5,{2210})
 */
bool CDFA::dfa_create(dfa_t *pdfa, char str[])
{
    int i, j, k, l, temp, new_state=0, str_num;
    int x, y, num, mode;

    // The pattern name.
    strcpy(pdfa->name, str);

    // Create the pattern from the string.
    for (i=0; str[i]!='\0' && strchr("$#+-|OoXx.?,!a*", *str); i++){
        memset(pdfa->states[new_state].next, 0, 4 * sizeof(int));
        str_num = find(str[i]);
        pdfa->states[new_state].next[str_num] = new_state + 1;
        pdfa->states[new_state+1].att = str_num;

        // Create skip table for states.
        for (l=0; l<4; l++){
            if (l == str_num)
                continue;
            for (j=new_state; j>=0; j--){

                // Next no matching state.
                if (pdfa->states[j].att == l){
                    temp = j;
                    for (k=1; j-k>=0 && pdfa->states[j-k].att == pdfa->states[new_state-k+1].att; k++){
                        if (pdfa->states[j-k].att == str_num)
                            temp = j-k;
                    }
                    if (j-k<1){
                        pdfa->states[new_state].next[l] = j;
                        break;
                    }
                    if (temp < j)
                        j = temp+1;
                }
            }
        }
        new_state++;

        // Realloc space if not enough.
        if (new_state >= pdfa->max_states)
            dfa_resize(pdfa, pdfa->max_states + Size, pdfa->max_indexes + Size);
    }
    pdfa->last_state = new_state;

    // Read the pattern from file, store them to the array.
    fscanf(m_partin,"%d",&num);
    for (i=0; i<num; i++){
        fscanf(m_partin,"%d",&mode);
        pdfa->indexes[i].mode = mode;
        // The pattern for the first point.
        if (mode == 1){
            fscanf(m_partin,"%d",&x);
            pdfa->indexes[i].offset[0] = x;
        }
        // The pattern for the second point.
        else{
            fscanf(m_partin,"%d %d",&x, &y);
            for ( ; mode>0; mode--){
                pdfa->indexes[i].offset[0] = x;
                pdfa->indexes[i].offset[1] = y;
            }
        }
    }
    pdfa->last_index = num;

    return 1;
}

// Realloc the space for the patterns.
void CDFA::dfa_resize(dfa_t *pdfa, int max_states, int max_indexes)
{
  state_t *pBuf;
  attrib_t *pBuf2;
  int i;

  pBuf = (state_t *)realloc(pdfa->states, max_states * sizeof(*pBuf));
  pBuf2 = (attrib_t *)realloc(pdfa->indexes, max_indexes * sizeof(*pBuf2));

  for (i = pdfa->max_states; i < max_states; i++)
    memset(pBuf + i, 0, sizeof(state_t));
  for (i = pdfa->max_indexes; i < max_indexes; i++)
    memset(pBuf2 + i, 0, sizeof(attrib_t));

  pdfa->states = pBuf;
  pdfa->max_states = max_states;
  pdfa->indexes = pBuf2;
  pdfa->max_indexes = max_indexes;
}

// Init the DFA, load the patterns from file.
// The first line is the number of the patterns in the file,
//   following the patterns.
bool CDFA::dfa_init()
{
    static const char *PATTERNS_IN = R"PAT(
34
**OO****    1   2 2 3
***OO***    1   2 2 5
**O*O***    1   2 2 4
**O**O**    1   2 3 4
**OO*O**    3   2 3 3  2 1 3  2 3 6
*O*O*O*     1   2 2 4
*O**OO***   3   2 1 5  2 0 2  2 0 5
O**O*O***   3   2 1 4  2 0 4  2 1 6
*O***O*O*   1   2 4 5
*O**O**O*   1   2 3 5
***OOO***   1    2 1 7
*OO****OO*  1   2 4 5
*OOO***OOO* 1   2 5 5
*OO****OOO* 2   2 5 7  2 5 6
**OOO***    3   2 2 2  2 1 2  2 2 6
*OOO**      3   1 5  1 1  1 0
OOO***      3   1 1  1 2  1 0
O*OO**      3   1 4  1 1  1 0
O**OO*      3   1 0  1 3  1 4
O***OO      3   1 2  1 3  1 4
OO*O**      3   1 0  1 1  1 3
OO**O*      3   1 0  1 2  1 3
OO***O      3   1 1  1 2  1 3
O*O**O      3   1 1  1 2  1 4
O*O*O*      3   1 2  1 4  1 0
*OO***OO    3   1 2  1 3  1 4
OOOO**      1   2 0 1
OOO*O*      1   2 0 2
OOO**O      1   2 1 2
OO*OO*      1   2 0 3
OO*O*O      1   2 1 3
OO**OO      1   2 2 3
O*OOO*      1   2 0 4
O*OO*O      1   2 1 4
)PAT";
    char input[100];
    m_partin = tmpfile();
    if (m_partin == NULL) return false;
    fputs(PATTERNS_IN, m_partin);
    rewind(m_partin);

    int max_index;
    if (fscanf(m_partin, "%d", &max_index) != 1) {
        fclose(m_partin);
        m_partin = NULL;
        return false;
    }
    m_dfa_index = 0;
    for (int i = 0; i < max_index; i++) {
        if (fscanf(m_partin, "%99s", input) != 1) break;
        memset(&m_dfa_array[m_dfa_index], 0, sizeof(m_dfa_array[0]));
        dfa_resize(&m_dfa_array[m_dfa_index], Size, Size);
        if (dfa_create(&m_dfa_array[m_dfa_index], input))
            m_dfa_index++;
    }
    fclose(m_partin);
    m_partin = NULL;
    return true;
}

// Free the space of the DFA.
void CDFA::dfa_kill()
{
    int i;
    for (i=0; i<m_dfa_index; i++){
        free(m_dfa_array[i].states);
        free(m_dfa_array[i].indexes);
        memset(&m_dfa_array[i], 0, sizeof(m_dfa_array[i]));
    }
}


// ======================= move_generator.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


// Scores for the move generator.
#define score_1         15000
#define score_1_5       5005
#define score_2         5000
#define score_2_5       4995
#define score_2_6       2000
#define score_2_9       900
#define score_3         800
#define score_3_1       750
#define score_3_2       300
#define score_3_3       200
#define score_3_5       145
#define score_3_6       144
#define score_4         140
#define score_4_5       139
#define score_4_6       137
#define score_4_7       90
#define score_4_8       80
#define score_5         60
#define score_5_5       50
#define score_6         40
#define score_6_5       35
#define score_6_6       30
#define score_6_7       25
#define score_7         8
#define score_8         3
#define score_9         1

#define V               0
#define T               1


CMoveGenerator::CMoveGenerator()
{
    m_dead_four_plus = 0;

    m_time_get_moves = 0.0;
    m_time_set_score = 0.0;
    m_time_test = 0.0;
}

int CMoveGenerator::get_move_list(char ourColor , move_t* moveList, char board[][GRID_NUM])
{
    static move_one_t moveOne[GRID_COUNT ] ;
    static move_one_t moveOneCopy[GRID_COUNT ] ;
    static move_one_t moveTwo[GRID_COUNT ];
    int i,j,n,m,ii;
    int count = 0;
    int numOfOne = NUMOFONE;
    int numOfTwo ;

    move_one_t newMoveOne[GRID_COUNT];

    init_valuable_space(board);

    int tempScore = 0;

    clock_t beg,end;
    beg = clock();

    m_pos_to_update.clear();
    n = set_score(ourColor,1,moveOne,board);
    numOfOne = n;

    //print_board();
    //print_score(moveOne, n);

    if ( numOfOne > NUMOFONE )
        numOfOne = NUMOFONE;

    if (m_dead_four_plus == 1)
    {
        numOfOne = 3;
    }

    for(i = 0 ; i< numOfOne ; i++)
    {
        // Get the second stone for a move.
        m_pos_to_update_special.clear();

        board[moveOne[i].x][moveOne[i].y] = ourColor;
        extend_pos(moveOne[i].x,moveOne[i].y, board);

        memcpy(moveOneCopy,moveOne,(n + 1)*sizeof(move_one_t));

        tempScore = moveOneCopy[i].score;

        moveOneCopy[i].score = 0;

        // Set scores for the second stone.
        m = set_score(ourColor,2,newMoveOne,board);

        numOfTwo = sort_merge(moveTwo,moveOneCopy,n,newMoveOne,m);
        m = numOfTwo;

        if (m > NUMOFTWO)
            m = NUMOFTWO;

        if (m_dead_four_plus == 1)
        {
            m = 12;
        }


        board[moveOne[i].x][moveOne[i].y] = NOSTONE;

        // Take the second store to make the moves.
        int    moreTwoMove = 0;

        for(j = 0 ; j < m && moveTwo[j].score; j++)
        {
            ii = 0;

            if (moveTwo[j].score == moveTwo[m].score && moveTwo[j].score)
            {
                m++;
                moreTwoMove++;
            }
            while (ii < count)
            {
                if (moveList[ii].positions[1].x == moveOne[i].x && moveList[ii].positions[1].y == moveOne[i].y &&
                        moveList[ii].positions[0].x == moveTwo[j].x && moveList[ii].positions[0].y == moveTwo[j].y )
                    break;
                ii++;
            }
            if (ii < count)
            {
                moreTwoMove--;

                if (moreTwoMove < 0 && m < numOfTwo)
                {
                    m++;
                    moreTwoMove++;
                }
                continue;
            }
            moveList[count].positions[0].x = moveOne[i].x;
            moveList[count].positions[0].y = moveOne[i].y;
            moveList[count].positions[1].x = moveTwo[j].x;
            moveList[count].positions[1].y = moveTwo[j].y;
            moveList[count++].score = moveTwo[j].score + tempScore;
        }
    }

    end = clock();
    m_time_get_moves += (double)(end-beg)/CLOCKS_PER_SEC;

    return count;
}

int CMoveGenerator::init_valuable_space(char board[][GRID_NUM]) {
    memset(map,0,sizeof(map));
    int count = 0;
    int i,j,x,y;

    for ( x = 1 ; x < GRID_NUM - 1; x++)
    {
        for ( y = 1 ; y < GRID_NUM - 1; y++)
        {
            if (board[x][y] != NOSTONE)
            {
                i = x - 1;
                j = y;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    i--;
                    count++;
                }
                i = x + 1;
                j = y;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    i++;
                    count++;
                }

                i = x;
                j = y - 1;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    j--;
                    count++;
                }
                i = x;
                j = y + 1;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    j++;
                    count++;
                }

                i = x - 1;
                j = y - 1;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    i--;
                    j--;
                    count++;
                }
                i = x + 1;
                j = y + 1;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    i++;
                    j++;
                    count++;
                }

                i = x - 1;
                j = y + 1;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    i--;
                    j++;
                    count++;
                }
                i = x + 1;
                j = y - 1;
                count = 0;
                while(count < 3 && board[i][j] == NOSTONE ) {
                    map[i][j] = 1;
                    i++;
                    j--;
                    count++;
                }
            } // if
        } // for y
    } // for x

    return 0;
}

void CMoveGenerator::add_new_pos_for_two(char x, char y)
{
    pos_t pos ;
    pos.x = x;
    pos.y = y;
    m_pos_to_update.push_back(pos);
}

void CMoveGenerator::add_new_pos_for_two_special(char x, char y)
{
    pos_t pos ;
    pos.x = x;
    pos.y = y;
    m_pos_to_update_special.push_back(pos);
}

bool CMoveGenerator::extend_pos(char x, char y, char board[][GRID_NUM])
{
    int i,j;
    int count;

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        i++;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }

        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        i--;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        j++;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        j--;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        i++;
        j++;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        i--;
        j--;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        i++;
        j--;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    i = x;j = y;
    count = 0;
    while(count < 3)
    {
        i--;
        j++;
        count++;
        if (board[i][j] == BORDER)
            break;
        if ( board[i][j] != NOSTONE)
        {
            count = 0;
            continue;
        }
        add_new_pos_for_two_special(i,j);
    }

    return true;
}

int CMoveGenerator::set_score( char ourColor , int step , move_one_t moveList[], char board[][GRID_NUM] )
{
    int count = 0 , score = 0;
    int i,j;
    int maxCount ;        // Return the max valid points in the board, sorted by their scores.

    int maxI;
    move_one_t tempMove;

    if ( step == 1 )
    {
        m_dead_four_plus = 0;
    }

    if (step == 1)
    {
        maxCount = NUMOFONE * 2;
        for (i = 1 ; i < GRID_NUM - 1 ; i++)
        {
            for (j = 1 ; j < GRID_NUM - 1 ; j++)
            {
                if (map[i][j] == 1)
                {
                    score = set_score_single( ourColor , i , j , step, board );

                    moveList[count].x = i;
                    moveList[count].y = j;
                    moveList[count++].score = score;
                }
            }
        }
    }else {
        maxCount = NUMOFTWO * 4;
        static char mapflag[GRID_NUM][GRID_NUM] ;
        memset(mapflag, 0, sizeof(mapflag));

        for (std::vector<pos_t>::iterator it = m_pos_to_update.begin(); it != m_pos_to_update.end() ; it++)
        {
            if (mapflag[it->x][it->y] || board[it->x][it->y] != NOSTONE)
            {
                continue;
            }
            else
            {
                mapflag[it->x][it->y] = 1;
            }
            score = set_score_single( ourColor , it->x , it->y , step, board );

            moveList[count].x = it->x;
            moveList[count].y = it->y;
            moveList[count++].score = score;
        }
        for (std::vector<pos_t>::iterator it = m_pos_to_update_special.begin(); it != m_pos_to_update_special.end() ; it++)
        {
            if (mapflag[it->x][it->y] || board[it->x][it->y] != NOSTONE)
            {
                continue;
            }
            else
            {
                mapflag[it->x][it->y] = 1;
            }
            score = set_score_single( ourColor , it->x , it->y , step, board );

            moveList[count].x = it->x;
            moveList[count].y = it->y;
            moveList[count++].score = score;
        }
    }

    clock_t beg,end;
    beg = clock();

    if (maxCount > count)
    {
        maxCount = count;
    }
    for(int i = 0 ; i < maxCount; i++)
    {
        maxI = i;
        for(int j = i+1 ; j < count ; j++)
        {
            if (moveList[maxI].score < moveList[j].score)
            {
                maxI = j;
            }
        }

        tempMove.score = moveList[maxI].score;
        tempMove.x = moveList[maxI].x;
        tempMove.y = moveList[maxI].y;
        moveList[maxI].score = moveList[i].score;
        moveList[maxI].x = moveList[i].x;
        moveList[maxI].y = moveList[i].y;
        moveList[i].x = tempMove.x;
        moveList[i].y = tempMove.y;
        moveList[i].score = tempMove.score;
    }

    end = clock();
    m_time_test += (double)(end-beg)/CLOCKS_PER_SEC;

    return count;
}

int CMoveGenerator::sort_merge(move_one_t list[],move_one_t listOne[],int oneN,move_one_t listTwo[],int twoN)
{
    int index[361] = {0};

    int count = 0,n;
    int i,j;

    for( i = 0 ; i < oneN ; i++)
    {
        index[(listOne[i].x - 1 ) * BOARD_SIZE + listOne[i].y - 1] = i ;
    }
    for( i = 0 ; i < twoN ; i++)
    {
        n = index[(listTwo[i].x - 1 ) * BOARD_SIZE + listTwo[i].y - 1];
        if (n + 1)
        {
            listOne[n].score = 0;
        }
    }
    for(  i = 0,j = 0 ; i < oneN &&j < twoN ; )
    {
        while(listOne[i].score == 0 && i < oneN)
        {
            i++;
        }

        if (listOne[i].score > listTwo[j].score)
        {
            list[count].score = listOne[i].score;
            list[count].x = listOne[i].x;
            list[count].y = listOne[i].y;
            i++;
            count++;
        }else
        {
            list[count].score = listTwo[j].score;
            list[count].x = listTwo[j].x;
            list[count].y = listTwo[j].y;
            j++;
            count++;
        }
        //if (count > 10)
        //    return ;
    }
    while (i < oneN)
    {
        if (listOne[i].score)
        {
            list[count].score = listOne[i].score;
            list[count].x = listOne[i].x;
            list[count].y = listOne[i].y;
            count++;
        }
        i++;
    }
    while (j < twoN)
    {
        list[count].score = listTwo[j].score;
        list[count].x = listTwo[j].x;
        list[count].y = listTwo[j].y;
        j++;
        count++;
    }

    return count;
}

int CMoveGenerator::set_score_single ( char ourColor , int x , int y , int step, char board[][GRID_NUM] )
{
    int score = 0;
    clock_t beg,end;
    beg = clock();

    score += set_by_direction1 ( ourColor , x , y , step, board );
    score += set_by_direction2 ( ourColor , x , y , step, board );
    score += set_by_direction3 ( ourColor , x , y , step, board );
    score += set_by_direction4 ( ourColor , x , y , step, board );
    score += g_board_base_score[x-1][y-1] + 1;

    end = clock();
    m_time_set_score += (double)(end-beg)/CLOCKS_PER_SEC;

    return score;
}

// Set score by vertical direction.
int CMoveGenerator::set_by_direction1 ( char color , int x , int y , int step, char board[][GRID_NUM] )
{
    // Set the color of self and enemy.
    int self , enemy ,
        highLevel , lowLevel , i ,
        connectCountUp = 0 , connectCountDn = 0 , OcountUp = 0 , OcountDn = 0 ,
        score = 0 , middleCheck[2] = {0,0} , connectCount = 0 ,
        defSuc = 0 , defVT = 0 , canGoFive = 0 , edgeBlock = 0 , Ocount = 0;
    if ( color == 1 )
    {
        self = 1;
        enemy = 2;
    }
    else
    {
        self = 2;
        enemy = 1;
    }

    OcountUp = 0;
    OcountDn = 0;
    connectCountUp = 0;
    connectCountDn = 0;
    connectCount = 0;
    highLevel = x;
    lowLevel = x;

    // Set the bounds of attack and defance.
    i = x;
    while ( OcountUp < 3 )
    {
        i --;
        if ( board[i][y] == enemy )
        {
            connectCountUp ++;
            middleCheck[0] = 1;
            continue;
        }
        else
        {
            if ( board[i][y] == self || !IsValidPos(i,y) )
            {
                highLevel = i;
                break;
            }
            else
            {
                if ( board[i][y] == 0 )
                {
                    if ( board[i-1][y] == 0 && board[i-2][y] != enemy )
                    {
                        highLevel = i;
                        break;
                    }
                    else
                    {
                        OcountUp ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountUp == 3 )
    {
        highLevel = i;
        OcountUp = 2;
    }

    // Set the bounds of attack and defance.
    i = x;
    while ( OcountDn < 3 )
    {
        i ++;
        if ( board[i][y] == enemy )
        {
            connectCountDn ++;
            middleCheck[1] = 1;
            continue;
        }
        else
        {
            if ( board[i][y] == self || board[i][y] == 3 )
            {
                lowLevel = i;
                break;
            }
            else
            {
                if ( board[i][y] == 0 )
                {
                    if ( board[i+1][y] == 0 && board[i+2][y] != enemy )
                    {
                        lowLevel = i;
                        break;
                    }
                    else
                    {
                        OcountDn ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountDn == 3 )
    {
        lowLevel = i;
        OcountDn = 2;
    }
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( lowLevel - x <= 6 || x - highLevel <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp + OcountDn + 1;
    }
    if ( middleCheck[0] && !middleCheck[1] )
    {
        if ( x - highLevel <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp;
    }
    if ( middleCheck[1] && !middleCheck[0] )
    {
        if ( lowLevel - x <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountDn;
    }
    if ( ( board[highLevel][y] == 3 || board[highLevel][y] == self ) && ( board[lowLevel][y] == 3 || board[lowLevel][y] == self ) )
    {
        if ( lowLevel - highLevel <= 6 )
        {
            defSuc = 0;
        }
    }
    connectCount = connectCountUp + connectCountDn;
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( Ocount == 1 && board[highLevel][y] == 0 && board[lowLevel][y] == 0 )
        {
            defVT = T;
        }
        else
        {
            defVT = V;
        }
    }
    else
    {
        if ( middleCheck[0] && !middleCheck[1] )
        {
            if ( OcountUp == 0 && board[highLevel][y] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountUp == 1 && board[x-1][y] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
        if ( !middleCheck[0] && middleCheck[1] )
        {
            if ( OcountDn == 0 && board[lowLevel][y] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountDn == 1 && board[x+1][y] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
    }


    if ( defSuc )
    {
        if ( connectCount == 1 )
        {
            score += score_9;
        }

        // Block the four stones.
        if ( connectCount == 4 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                score += score_2;
                if ( step == 1 )
                {
                    if ( Ocount <= 2 )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
            else
            {
                if ( step == 2 )
                {
                    if ( defVT == V )
                    {
                        score += score_2;
                    }
                }
                else
                {
                    score += score_2;
                    if ( defVT == V )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
        }
        if ( connectCount == 1 )
        {
            if ( board[x-1][y] == 0 && board[x-2][y] == enemy && board[x-3][y] == 0 && board[x-4][y] == 0 && board[x-5][y] == enemy && board[x-6][y] == 0 && board[x-7][y] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( board[x+1][y] == 0 && board[x+2][y] == enemy && board[x+3][y] == 0 && board[x+4][y] == 0 && board[x+5][y] == enemy && board[x+6][y] == 0 && board[x+7][y] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
        }

        // Block the two stones.
        if ( connectCount == 2 )
        {
            if ( middleCheck[0] && middleCheck[1] && board[highLevel][y] == 0 && board[lowLevel][y] == 0 && Ocount <= 2 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( middleCheck[0] && !middleCheck[1] && board[highLevel][y] == 0 )
            {
                if ( board[x-1][y] == 0 && board[x-2][y] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x-1][y] == enemy && board[x-2][y] == 0 && board[x-3][y] == 0 && board[x-4][y] == enemy && board[x-5][y] == 0 && ( board[x-6][y] == self || board[x-6][y] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x-1][y] == 0 && board[x-2][y] == enemy && board[x-3][y] == 0 && board[x-4][y] == enemy && board[x-5][y] == 0 && ( board[x-6][y] == self || board[x-6][y] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x-1][y] == enemy && ( board[x+1][y] == self || board[x+1][y] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( middleCheck[1] && !middleCheck[0] && board[lowLevel][y] == 0 )
            {
                if ( board[x+1][y] == 0 && board[x+2][y] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x+1][y] == enemy && board[x+2][y] == 0 && board[x+3][y] == 0 && board[x+4][y] == enemy && board[x+5][y] == 0 && ( board[x+6][y] == self || board[x+6][y] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x+1][y] == 0 && board[x+2][y] == enemy && board[x+3][y] == 0 && board[x+4][y] == enemy && board[x+5][y] == 0 && ( board[x+6][y] == self || board[x+6][y] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x+1][y] == enemy && ( board[x-1][y] == self || board[x-1][y] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
        // Block the three stones.
        if ( connectCount == 3 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 200
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //137
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( middleCheck[0] && !middleCheck[1] && ( board[x-1][y] == enemy || board[x-2][y] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( !middleCheck[0] && middleCheck[1] && ( board[x+1][y] == enemy || board[x+2][y] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

        }
        // Block five stones.
        if ( connectCount == 5 )
        {
            score += score_2;
        }
        // Block six.
        if ( connectCount == 6 )
        {
            score += score_2;
        }
        // Block seven.
        if ( connectCount == 7 )
        {
            score += score_2;
        }
        // Block eight.
        if ( connectCount == 8 )
        {
            score += score_2;
        }
        // Block nine.
        if ( connectCount == 9 )
        {
            score += score_2;
        }
        // Block ten.
        if ( connectCount == 10 )
        {
            score += score_2;
        }
    }
    else
    {
        score = 0;
    }


    // Set the bounds of attack.
    connectCount = 0;
    Ocount = 0;
    highLevel = x;
    lowLevel = x;
    i = x;
    int sixDecreaseO_1 = 0 , sixDecreaseO_2 = 0 , sixDecreaseO = 0;
    while ( 1 )
    {
        if ( board[i][y] == self )
        {
            connectCount ++;
        }
        if ( board[i][y] == enemy )
        {
            highLevel = i;
            break;
        }
        else
        {
            if ( board[i][y] == 0 )
            {
                if ( !IsValidPos(i-1,y) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    highLevel = i-1;
                    sixDecreaseO_1 = 1;
                }
                else
                {
                    if ( !IsValidPos(i-2,y) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        highLevel = i;
                        if ( i == x )
                        {
                            if ( board[i-1][y] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[i-1][y] == enemy )
                            {
                                highLevel = i - 1;
                            }
                            if ( board[i-1][y] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[i-1][y] == enemy )
                            {
                                highLevel = i - 1;
                                Ocount ++;
                            }
                            if ( board[i-1][y] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[i-1][y] == enemy )
                {
                    highLevel = i-1;
                    canGoFive = 1;
                    if ( i != x )
                    {
                        Ocount ++;
                        sixDecreaseO_1 = 1;
                    }
                    break;
                }
                if ( board[i-1][y] == 0 && board[i-2][y] == enemy )
                {
                    highLevel = i;
                    if ( i== x )
                    {
                        Ocount ++;
                        sixDecreaseO_1 = 1;
                    }

                    break;
                }

                if ( board[i-1][y] == 0 && board[i-2][y] == 0 )
                {
                    highLevel = i;
                    if ( i == x )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(i,y) )
                {
                    edgeBlock = 1;
                    highLevel = i;
                    break;
                }
            }
        }
        i--;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x-1][y] == 0 && ( board[x-2][y] == enemy || board[x-2][y] == 3 ) )
    {
        highLevel = x-2;
    }

    i = x;
    while ( 1 )
    {
        if ( board[i][y] == self )
        {
            connectCount ++;
        }
        if ( board[i][y] == enemy )
        {
            lowLevel = i;
            break;
        }
        else
        {
            if ( board[i][y] == 0 )
            {
                if ( !IsValidPos(i+1,y) )
                {
                    edgeBlock = 1;  // Blocked by the edges.
                    canGoFive = 1;
                    sixDecreaseO_2 = 1;

                }
                else
                {
                    if ( !IsValidPos(i+2,y) )
                    {
                        edgeBlock = 1;  // Blocked by the edges.
                        canGoFive = 1;
                        lowLevel = i;
                        if ( i == x )
                        {
                            if ( board[i+1][y] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[i+1][y] == enemy )
                            {
                                lowLevel = i + 1;
                            }
                            if ( board[i+1][y] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[i+1][y] == enemy )
                            {
                                lowLevel = i + 1;
                                Ocount ++;
                            }
                            if ( board[i+1][y] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[i+1][y] == enemy )
                {
                    lowLevel = i+1;
                    canGoFive = 1;
                    if ( i != x )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i+1][y] == 0 && board[i+2][y] == enemy )
                {
                    lowLevel = i;
                    if ( i == x )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i+1][y] == 0 && board[i+2][y] == 0 )
                {
                    lowLevel = i;
                    if ( i == x )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(i,y) )
                {
                    edgeBlock = 1;
                    highLevel = i;
                    break;
                }
            }
        }
        i++;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x+1][y] == 0 && ( board[x+2][y] == enemy || board[x+2][y] == 3 ) )
    {
        highLevel = x+2;
    }
    sixDecreaseO = sixDecreaseO_1 + sixDecreaseO_2;

    // Add scores for connected stones.
    if ( connectCount == 1 && board[lowLevel][y] == 0 && board[highLevel][y] == 0 )
    {
        if ( Ocount == 0 )
        {
            score += score_6;           // Connect for two.
        }
        else
        {
            if ( Ocount == 1 )
            {
                score += score_6_5;
            }
            else
            {
                if ( Ocount == 2)
                {
                    score += score_6_6;
                }
            }
        }
    }
    // For make three connection.
    if ( connectCount == 2 )
    {
        if ( board[highLevel][y] == 0 && board[lowLevel][y] == 0 )
        {
            if ( Ocount < 2 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount == 2 )
            {
                score += score_6_6;
            }
        }
        if ( ( board[highLevel][y] == 0 && board[lowLevel][y] != 0 && board[lowLevel][y] != self ) || ( board[highLevel][y] != 0 &&  board[highLevel][y] != self && board[lowLevel][y] == 0 ) )
        {
            score += score_6_6;
        }
        if ( board[lowLevel][y] != 0 && board[lowLevel][y] != self && board [highLevel][y] != 0 && board [highLevel][y] != self && lowLevel - highLevel >= 7 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_6_6;
            }
        }
    }
    // To make four.
    if ( connectCount == 3 )
    {
        if ( board[lowLevel][y] == 0 && board[highLevel][y] == 0 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_2_9;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_3;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_3_2;
            }
        }
        if ( ( board[lowLevel][y] == 0 && board[highLevel][y] != 0 && board[highLevel][y] != self ) || ( board[lowLevel][y] != 0 && board[lowLevel][y] != self && board[highLevel][y] == 0 ) )
        {
            if ( Ocount <= 2 )
            {
                score += score_3_2;
            }
        }
        if ( board[lowLevel][y] != 0 && board[lowLevel][y] != self && board[highLevel][y] != 0 && board[highLevel][y] != self && lowLevel - highLevel >= 7 )
        {
            if ( Ocount <= 2 )
            {
                score += score_3_2;
            }
        }
    }
    if ( connectCount >= 4 )
    {
        // To make five.
        if ( step == 1 )
        {
            // OOOO to make five.
            if ( board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self && board[x-4][y] == self && ( board[x-5][y] == 0 || board[x+1][y] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self && board[x+4][y] == self && ( board[x+5][y] == 0 || board[x-1][y] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            // OXOOO to make five.
            if ( board[x-1][y] == self && board[x-2][y] == 0 && board[x-3][y] == self && board[x-4][y] == self && board[x-5][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x+2][y] == 0 && board[x+3][y] == self && board[x+4][y] == self && board[x+5][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y] == self && board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self && ( board[x+4][y] == 0 || board[x-2][y] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self && ( board[x-4][y] == 0 || board[x+2][y] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self && board[x-4][y] == 0 && board[x-5][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self && board[x+4][y] == 0 && board[x+5][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            // OOXOO
            if ( board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == 0 && board[x-4][y] == self && board[x-5][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == 0 && board[x+4][y] == self && board[x+5][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y] == self && board[x-2][y] == self && board[x+1][y] == self && board[x+2][y] == self && ( board[x-3][y] == 0 || board[x+3][y] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }

            // OXXOOO
            if ( board[x-1][y] == self && board[x+1][y] == 0 && board[x+2][y] == self && board[x+3][y] == self && board[x+4][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x-1][y] == 0 && board[x-2][y] == self && board[x-3][y] == self && board[x-4][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y] == 0 && board[x-2][y] == self && board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == 0 && board[x+2][y] == self && board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            // OOXXOO
            if ( board[x-1][y] == self && board[x-2][y] == self && board[x+1][y] == 0 && board[x+2][y] == self && board[x+3][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x+2][y] == self && board[x-1][y] == 0 && board[x-2][y] == self && board[x-3][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            // OXOXOO
            if ( board[x-1][y] == self && board[x+1][y] == self && board[x+2][y] == 0 && board[x+3][y] == self && board[x+4][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x-1][y] == self && board[x-2][y] == 0 && board[x-3][y] == self && board[x-4][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y] == self && board[x-2][y] == 0 && board[x-3][y] == self && board[x+1][y] == self && board[x+2][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x+2][y] == 0 && board[x+3][y] == self && board[x-1][y] == self && board[x-2][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            // OXOOXO
            if ( board[x-1][y] == self && board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == 0 && board[x+4][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y] == self && board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == 0 && board[x-4][y] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
        }
        else
        {
            if ( board[x+1][y] == 0 && board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self && board[x-4][y] == self && board[x-5][y] == 0 && ( board[x-6][y] == enemy || board[x-6][y] == 3 ) )
            {
                score += score_3;
            }
            else
            {
                if ( board[x-1][y] == 0 && board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self && board[x+4][y] == self && board[x+5][y] == 0 && ( board[x+6][y] == enemy || board[x+6][y] == 3 ) )
                {
                    score += score_3;
                }
                else
                {
                    score += score_6_6;
                }
            }
        }

        // OOOOO
        if ( board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self && board[x-4][y] == self && board[x-5][y] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self && board[x+4][y] == self && board[x+5][y] == self )
        {
            score += score_1;
        }
        // OXOOOO
        if ( board[x-1][y] == self && board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self && board[x+4][y] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y] == self && board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self && board[x-4][y] == self )
        {
            score += score_1;
        }
        // OOXOOO
        if ( board[x-1][y] == self && board[x-2][y] == self && board[x+1][y] == self && board[x+2][y] == self && board[x+3][y] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y] == self && board[x+2][y] == self && board[x-1][y] == self && board[x-2][y] == self && board[x-3][y] == self )
        {
            score += score_1;
        }
    }
    return score;
}

// Set score by left up direction.
int CMoveGenerator::set_by_direction2 ( char color , int x , int y , int step, char board[][GRID_NUM] )
{
    int self , enemy , highLevelx , lowLevelx , highLevely , lowLevely , i , j , connectCountUp = 0 , connectCountDn = 0 , OcountUp = 0 , OcountDn = 0 , score = 0 , middleCheck[2] = {0,0} , connectCount = 0 , defSuc = 0 , defVT = 0 , canGoFive = 0 , edgeBlock = 0 , Ocount = 0;

    if ( color == 1 )
    {
        self = 1;
        enemy = 2;
    }
    else
    {
        self = 2;
        enemy = 1;
    }

    OcountUp = 0;
    OcountDn = 0;
    connectCountUp = 0;
    connectCountDn = 0;
    connectCount = 0;
    highLevelx = x;
    highLevely = y;
    lowLevelx = x;
    lowLevely = y;

    i = x;
    j = y;
    while ( OcountUp < 3 )
    {
        i --;
        j ++;
        if ( board[i][j] == enemy )
        {
            connectCountUp ++;
            middleCheck[0] = 1;
            continue;
        }
        else
        {
            if ( board[i][j] == self || !IsValidPos(i,j) )
            {
                highLevelx = i;
                highLevely = j;
                break;
            }
            else
            {
                if ( board[i][j] == 0 )
                {
                    if ( board[i-1][j+1] == 0 && board[i-2][j+2] != enemy )
                    {
                        highLevelx = i;
                        highLevely = j;
                        break;
                    }
                    else
                    {
                        OcountUp ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountUp == 3 )
    {
        highLevelx = i;
        highLevely = j;
        OcountUp = 2;
    }
    i = x;
    j = y;
    while ( OcountDn < 3 )
    {
        i ++;
        j --;
        if ( board[i][j] == enemy )
        {
            connectCountDn ++;
            middleCheck[1] = 1;
            continue;
        }
        else
        {
            if ( board[i][j] == self || board[i][j] == 3 )
            {
                lowLevelx = i;
                lowLevely = j;
                break;
            }
            else
            {
                if ( board[i][j] == 0 )
                {
                    if ( board[i+1][j-1] == 0 && board[i+2][j-2] != enemy )
                    {
                        lowLevelx = i;
                        lowLevely = j;
                        break;
                    }
                    else
                    {
                        OcountDn ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountDn == 3 )
    {
        lowLevelx = i;
        lowLevely = j;
        OcountDn = 2;
    }
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( lowLevelx - x <= 6 || x - highLevelx <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp + OcountDn + 1;
    }
    if ( middleCheck[0] && !middleCheck[1] )
    {
        if ( x - highLevelx <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp;
    }
    if ( middleCheck[1] && !middleCheck[0] )
    {
        if ( lowLevelx - x <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountDn;
    }
    if ( ( board[highLevelx][highLevely] == 3 || board[highLevelx][highLevely] == self ) && ( board[lowLevelx][lowLevely] == 3 || board[lowLevelx][lowLevely] == self ) )
    {
        if ( lowLevelx - highLevelx <= 6 )
        {
            defSuc = 0;
        }
    }
    connectCount = connectCountUp + connectCountDn;
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( Ocount == 1 && board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] == 0 )
        {
            defVT = T;
        }
        else
        {
            defVT = V;
        }
    }
    else
    {
        if ( middleCheck[0] && !middleCheck[1] )
        {
            if ( OcountUp == 0 && board[highLevelx][highLevely] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountUp == 1 && board[x-1][y+1] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
        if ( !middleCheck[0] && middleCheck[1] )
        {
            if ( OcountDn == 0 && board[lowLevelx][lowLevely] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountDn == 1 && board[x+1][y-1] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
    }
    if ( defSuc )
    {
        if ( connectCount == 1 )
        {
            score += score_9;
        }
        if ( connectCount == 4 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                score += score_2;
                if ( step == 1 )
                {
                    if ( Ocount <= 2 )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
            else
            {
                if ( step == 2 )
                {
                    if ( defVT == V )
                    {
                        score += score_2;
                    }
                }
                else
                {
                    score += score_2;
                    if ( defVT == V )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
        }
        if ( connectCount == 1 )
        {
            if ( board[x-1][y+1] == 0 && board[x-2][y+2] == enemy && board[x-3][y+3] == 0 && board[x-4][y+4] == 0 && board[x-5][y+5] == enemy && board[x-6][y+6] == 0 && board[x-7][y+7] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( board[x+1][y-1] == 0 && board[x+2][y-2] == enemy && board[x+3][y-3] == 0 && board[x+4][y-4] == 0 && board[x+5][y-5] == enemy && board[x+6][y-6] == 0 && board[x+7][y-7] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
        }
        if ( connectCount == 2 )
        {
            if ( middleCheck[0] && middleCheck[1] && board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] == 0 && Ocount <= 2 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( middleCheck[0] && !middleCheck[1] && board[highLevelx][highLevely] == 0 )
            {
                if ( board[x-1][y+1] == 0 && board[x-2][y+2] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x-1][y+1] == enemy && board[x-2][y+2] == 0 && board[x-3][y+3] == 0 && board[x-4][y+4] == enemy && board[x-5][y+5] == 0 && ( board[x-6][y+6] == self || board[x-6][y+6] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x-1][y+1] == 0 && board[x-2][y+2] == enemy && board[x-3][y+3] == 0 && board[x-4][y+4] == enemy && board[x-5][y+5] == 0 && ( board[x-6][y+6] == self || board[x-6][y+6] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x-1][y+1] == enemy && ( board[x+1][y-1] == self || board[x+1][y-1] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( middleCheck[1] && !middleCheck[0] && board[lowLevelx][lowLevely] == 0 )
            {
                if ( board[x+1][y-1] == 0 && board[x+2][y-2] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x+1][y-1] == enemy && board[x+2][y-2] == 0 && board[x+3][y-3] == 0 && board[x+4][y-4] == enemy && board[x+5][y-5] == 0 && ( board[x+6][y-6] == self || board[x+6][y-6] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x+1][y-1] == 0 && board[x+2][y-2] == enemy && board[x+3][y-3] == 0 && board[x+4][y-4] == enemy && board[x+5][y-5] == 0 && ( board[x+6][y-6] == self || board[x+6][y-6] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x+1][y-1] == enemy && ( board[x-1][y+1] == self || board[x-1][y+1] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
        if ( connectCount == 3 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 200
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //137
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( middleCheck[0] && !middleCheck[1] && ( board[x-1][y+1] == enemy || board[x-2][y+2] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( !middleCheck[0] && middleCheck[1] && ( board[x+1][y-1] == enemy || board[x+2][y-2] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

        }
        if ( connectCount == 5 )
        {
            score += score_2;
        }
        if ( connectCount == 6 )
        {
            score += score_2;
        }
        if ( connectCount == 7 )
        {
            score += score_2;
        }
        if ( connectCount == 8 )
        {
            score += score_2;
        }
        if ( connectCount == 9 )
        {
            score += score_2;
        }
        if ( connectCount == 10 )
        {
            score += score_2;
        }
    }
    else
    {
        score = 0;
    }

    i = x;
    j = y;
    highLevelx = x;
    lowLevelx = x;
    highLevely = y;
    lowLevely = y;
    connectCount = 0;
    Ocount = 0;
    int sixDecreaseO = 0 , sixDecreaseO_1 = 0 , sixDecreaseO_2 = 0;
    while ( 1 )
    {
        if ( board[i][j] == self )
        {
            connectCount ++;
        }
        if ( board[i][j] == enemy )
        {
            highLevelx = i;
            highLevely = j;
            break;
        }
        else
        {
            if ( board[i][j] == 0 )
            {
                if ( !IsValidPos(i-1,j+1) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    sixDecreaseO_1 = 0;
                }
                else
                {
                    if ( !IsValidPos(i-2,j+2) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        highLevelx = i;
                        highLevely = j;
                        if ( i == x )
                        {
                            if ( board[i-1][j+1] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[i-1][j+1] == enemy )
                            {
                                highLevelx = i - 1;
                                highLevely = j + 1;
                            }
                            if ( board[i-1][j+1] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[i-1][j+1] == enemy )
                            {
                                highLevelx = i - 1;
                                highLevely = j + 1;
                                Ocount ++;
                            }
                            if ( board[i-1][j+1] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[i-1][j+1] == enemy )
                {
                    highLevelx = i-1;
                    highLevely = j+1;
                    canGoFive = 1;
                    if ( i != x )
                    {
                        sixDecreaseO_1 = 0;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i-1][j+1] == 0 && board[i-2][j+2] == enemy )
                {
                    highLevelx = i;
                    highLevely = j;
                    if ( i == x )
                    {
                        sixDecreaseO_1 = 0;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i-1][j+1] == 0 && board[i-2][j+2] == 0 )
                {
                    highLevelx = i;
                    highLevely = j;
                    if ( i == x )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(i,j) )
                {
                    edgeBlock = 1;
                    highLevelx = i;
                    highLevely = j;
                    break;
                }
            }
        }
        i--;
        j++;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x-1][y+1] == 0 && ( board[x-2][y+2] == enemy || board[x-2][y+2] == 3 ) )
    {
        highLevelx = x-2;
        highLevely = y+2;
    }

    i = x;
    j = y;
    while ( 1 )
    {
        if ( board[i][j] == self )
        {
            connectCount ++;
        }
        if ( board[i][j] == enemy )
        {
            lowLevelx = i;
            lowLevely = j;
            break;
        }
        else
        {
            if ( board[i][j] == 0 )
            {
                if ( !IsValidPos(i+1,j-1) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    sixDecreaseO_2 = 1;
                }
                else
                {
                    if ( !IsValidPos(i+2,j-2) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        lowLevelx = i;
                        lowLevely = j;
                        if ( i == x )
                        {
                            if ( board[i+1][y-1] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[i+1][j-1] == enemy )
                            {
                                lowLevelx = i + 1;
                                lowLevely = j - 1;
                            }
                            if ( board[i+1][j-1] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[i+1][j-1] == enemy )
                            {
                                highLevelx = i + 1;
                                highLevely = j - 1;
                                Ocount ++;
                            }
                            if ( board[i+1][j-1] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[i+1][j-1] == enemy )
                {
                    lowLevelx = i+1;
                    lowLevely = j-1;
                    canGoFive = 1;
                    if ( i != x )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i+1][j-1] == 0 && board[i+2][j-2] == enemy )
                {
                    lowLevelx = i;
                    lowLevely = j;
                    if ( i == x )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i+1][j-1] == 0 && board[i+2][j-2] == 0 )
                {
                    lowLevelx = i;
                    lowLevely = j;
                    if ( i == x )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(i,j) )
                {
                    edgeBlock = 1;
                    lowLevelx = i;
                    lowLevely = j;
                    break;
                }
            }
        }
        i++;
        j--;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x+1][y-1] == 0 && ( board[x+2][y-2] == enemy || board[x+2][y-2] == 3 ) )
    {
        lowLevelx = x+2;
        lowLevely = y-2;
    }
    sixDecreaseO = sixDecreaseO_1 + sixDecreaseO_2;
    if ( connectCount == 1 && board[lowLevelx][lowLevely] == 0 && board[highLevelx][highLevely] == 0 )
    {
        if ( Ocount == 0 )
        {
            score += score_6;           //
        }
        else
        {
            if ( Ocount == 1 )
            {
                score += score_6_5;      //
            }
            else
            {
                if ( Ocount == 2)
                {
                    score += score_6_6; //
                }
            }
        }
    }
    if ( connectCount == 2 )
    {
        if ( board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] == 0 )
        {
            if ( Ocount < 2 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount == 2 )
            {
                score += score_6_6;
            }
        }
        if ( ( board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self  ) || ( board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self && board[lowLevelx][lowLevely] == 0 ) )
        {
            score += score_6_6;
        }
        if ( board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self && board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self && lowLevelx - highLevelx >= 7 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_6_6;
            }
        }
    }
    if ( connectCount == 3 )
    {
        if ( board[lowLevelx][lowLevely] == 0 && board[highLevelx][highLevely] == 0 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_2_9;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_3;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_3_2;
            }
        }
        if ( ( board[lowLevelx][lowLevely] == 0 && board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self ) || ( board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self && board[highLevelx][highLevely] == 0 ) )
        {
            if ( Ocount <= 2 )
            {
                score += score_3_2;
            }
        }
        if ( board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self && board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self && lowLevelx - highLevelx >= 7 )
        {
            if ( Ocount <= 2 )
            {
                score += score_3_2;
            }
        }
    }
    if ( connectCount >= 4 )
    {
        if ( step == 1 )
        {
            if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self && board[x-4][y+4] == self && ( board[x-5][y+5] == 0 || board[x+1][y-1] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self && board[x+4][y-4] == self && ( board[x+5][y-5] == 0 || board[x-1][y+1] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x-2][y+2] == 0 && board[x-3][y+3] == self && board[x-4][y+4] == self && board[x-5][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x+2][y-2] == 0 && board[x+3][y-3] == self && board[x+4][y-4] == self && board[x+5][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self && ( board[x+4][y-4] == 0 || board[x-2][y+2] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self && ( board[x-4][y+4] == 0 || board[x+2][y-2] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self && board[x-4][y+4] == 0 && board[x-5][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self && board[x+4][y-4] == 0 && board[x+5][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == 0 && board[x-4][y+4] == self && board[x-5][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == 0 && board[x+4][y-4] == self && board[x+5][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x+1][y-1] == self && board[x+2][y-2] == self && ( board[x-3][y+3] == 0 || board[x+3][y-3] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }

            if ( board[x-1][y+1] == self && board[x+1][y-1] == 0 && board[x+2][y-2] == self && board[x+3][y-3] == self && board[x+4][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x-1][y+1] == 0 && board[x-2][y+2] == self && board[x-3][y+3] == self && board[x-4][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == 0 && board[x-2][y+2] == self && board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == 0 && board[x+2][y-2] == self && board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x+1][y-1] == 0 && board[x+2][y-2] == self && board[x+3][y-3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x+2][y-2] == self && board[x-1][y+1] == 0 && board[x-2][y+2] == self && board[x-3][y+3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x+1][y-1] == self && board[x+2][y-2] == 0 && board[x+3][y-3] == self && board[x+4][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x-1][y+1] == self && board[x-2][y+2] == 0 && board[x-3][y+3] == self && board[x-4][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x-2][y+2] == 0 && board[x-3][y+3] == self && board[x+1][y-1] == self && board[x+2][y-2] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x+2][y-2] == 0 && board[x+3][y-3] == self && board[x-1][y+1] == self && board[x-2][y+2] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y+1] == self && board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == 0 && board[x+4][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y-1] == self && board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == 0 && board[x-4][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
        }
        else
        {
            if ( board[x+1][y-1] == 0 && board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self && board[x-4][y+4] == self && board[x-5][y+5] == 0 && ( board[x-6][y+6] == enemy || board[x-6][y+6] == 3 ) )
            {
                score += score_3;
            }
            else
            {
                if ( board[x-1][y+1] == 0 && board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self && board[x+4][y-4] == self && board[x+5][y-5] == 0 && ( board[x+6][y-6] == enemy || board[x+6][y-6] == 3 ) )
                {
                    score += score_3;
                }
                else
                {
                    score += score_6_6;
                }
            }
        }

        if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self && board[x-4][y+4] == self && board[x-5][y+5] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self && board[x+4][y-4] == self && board[x+5][y-5] == self )
        {
            score += score_1;
        }
        if ( board[x-1][y+1] == self && board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self && board[x+4][y-4] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y-1] == self && board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self && board[x-4][y+4] == self )
        {
            score += score_1;
        }
        if ( board[x-1][y+1] == self && board[x-2][y+2] == self && board[x+1][y-1] == self && board[x+2][y-2] == self && board[x+3][y-3] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y-1] == self && board[x+2][y-2] == self && board[x-1][y+1] == self && board[x-2][y+2] == self && board[x-3][y+3] == self )
        {
            score += score_1;
        }
    }


    return score;
}


// Set score by horizon direction.
int CMoveGenerator::set_by_direction3 ( char color , int x , int y , int step, char board[][GRID_NUM] )
{
    int self , enemy , highLevel , lowLevel , j , connectCountUp = 0 , connectCountDn = 0 , OcountUp = 0 , OcountDn = 0 , score = 0 , middleCheck[2] = {0,0} , connectCount = 0 , defSuc = 0 , defVT = 0 , canGoFive = 0 , edgeBlock = 0 , Ocount = 0;

    if ( color == 1 )
    {
        self = 1;
        enemy = 2;
    }
    else
    {
        self = 2;
        enemy = 1;
    }

    OcountUp = 0;
    OcountDn = 0;
    connectCountUp = 0;
    connectCountDn = 0;
    connectCount = 0;
    highLevel = x;
    lowLevel = x;

    j = y;
    while ( OcountUp < 3 )
    {
        j ++;
        if ( board[x][j] == enemy )
        {
            connectCountUp ++;
            middleCheck[0] = 1;
            continue;
        }
        else
        {
            if ( board[x][j] == self || !IsValidPos(x,j) )
            {
                highLevel = j;
                break;
            }
            else
            {
                if ( board[x][j] == 0 )
                {
                    if ( board[x][j+1] == 0 && board[x][j+2] != enemy )
                    {
                        highLevel = j;
                        break;
                    }
                    else
                    {
                        OcountUp ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountUp == 3 )
    {
        highLevel = j;
        OcountUp = 2;
    }

    j = y;
    while ( OcountDn < 3 )
    {
        j --;
        if ( board[x][j] == enemy )
        {
            connectCountDn ++;
            middleCheck[1] = 1;
            continue;
        }
        else
        {
            if ( board[x][j] == self || board[x][j] == 3 )
            {
                lowLevel = j;
                break;
            }
            else
            {
                if ( board[x][j] == 0 )
                {
                    if ( board[x][j-1] == 0 && board[x][j-2] != enemy )
                    {
                        lowLevel = j;
                        break;
                    }
                    else
                    {
                        OcountDn ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountDn == 3 )
    {
        lowLevel = j;
        OcountDn = 2;
    }
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( y - lowLevel <= 6 || highLevel - y <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp + OcountDn + 1;
    }
    if ( middleCheck[0] && !middleCheck[1] )
    {
        if ( highLevel - y <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp;
    }
    if ( middleCheck[1] && !middleCheck[0] )
    {
        if ( y - lowLevel <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountDn;
    }
    if ( ( board[x][highLevel] == 3 || board[x][highLevel] == self ) && ( board[x][lowLevel] == 3 || board[x][lowLevel] == self ) )
    {
        if ( highLevel - lowLevel <= 6 )
        {
            defSuc = 0;
        }
    }
    connectCount = connectCountUp + connectCountDn;
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( Ocount == 1 && board[x][highLevel] == 0 && board[x][lowLevel] == 0 )
        {
            defVT = T;
        }
        else
        {
            defVT = V;
        }
    }
    else
    {
        if ( middleCheck[0] && !middleCheck[1] )
        {
            if ( OcountUp == 0 && board[x][highLevel] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountUp == 1 && board[x][y+1] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
        if ( !middleCheck[0] && middleCheck[1] )
        {
            if ( OcountDn == 0 && board[x][lowLevel] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountDn == 1 && board[x][y-1] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
    }
    if ( defSuc )
    {
        if ( connectCount == 1 )
        {
            score += score_9;
        }
        if ( connectCount == 4 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                score += score_2;
                if ( step == 1 )
                {
                    if ( Ocount <= 2 )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
            else
            {
                if ( step == 2 )
                {
                    if ( defVT == V )
                    {
                        score += score_2;
                    }
                }
                else
                {
                    score += score_2;
                    if ( defVT == V )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
        }
        if ( connectCount == 1 )
        {
            if ( board[x][y-1] == 0 && board[x][y-2] == enemy && board[x][y-3] == 0 && board[x][y-4] == 0 && board[x][y-5] == enemy && board[x][y-6] == 0 && board[x][y-7] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( board[x][y+1] == 0 && board[x][y+2] == enemy && board[x][y+3] == 0 && board[x][y+4] == 0 && board[x][y+5] == enemy && board[x][y+6] == 0 && board[x][y+7] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
        }
        if ( connectCount == 2 )
        {
            if ( middleCheck[0] && middleCheck[1] && board[x][highLevel] == 0 && board[x][lowLevel] == 0 && Ocount <= 2 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( middleCheck[0] && !middleCheck[1] && board[x][highLevel] == 0 )
            {
                if ( board[x][y+1] == 0 && board[x][y+2] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x][y+1] == enemy && board[x][y+2] == 0 && board[x][y+3] == 0 && board[x][y+4] == enemy && board[x][y+5] == 0 && ( board[x][y+6] == self || board[x][y+6] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x][y+1] == 0 && board[x][y+2] == enemy && board[x][y+3] == 0 && board[x][y+4] == enemy && board[x][y+5] == 0 && ( board[x][y+6] == self || board[x][y+6] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x][y+1] == enemy && ( board[x][y-1] == self || board[x][y-1] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( middleCheck[1] && !middleCheck[0] && board[x][lowLevel] == 0 )
            {
                if ( board[x][y-1] == 0 && board[x][y-2] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x][y-1] == enemy && board[x][y-2] == 0 && board[x][y-3] == 0 && board[x][y-4] == enemy && board[x][y-5] == 0 && ( board[x][y-6] == self || board[x][y-6] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x][y-1] == 0 && board[x][y-2] == enemy && board[x][y-3] == 0 && board[x][y-4] == enemy && board[x][y-5] == 0 && ( board[x][y-6] == self || board[x][y-6] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x][y-1] == enemy && ( board[x][y+1] == self || board[x][y+1] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
        if ( connectCount == 3 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 200
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //137
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( middleCheck[0] && !middleCheck[1] && ( board[x][y+1] == enemy || board[x][y+2] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( !middleCheck[0] && middleCheck[1] && ( board[x][y-1] == enemy || board[x][y-2] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

        }
        if ( connectCount == 5 )
        {
            score += score_2;
        }
        if ( connectCount == 6 )
        {
            score += score_2;
        }
        if ( connectCount == 7 )
        {
            score += score_2;
        }
        if ( connectCount == 8 )
        {
            score += score_2;
        }
        if ( connectCount == 9 )
        {
            score += score_2;
        }
        if ( connectCount == 10 )
        {
            score += score_2;
        }
    }
    else
    {
        score = 0;
    }
    j = y;
    highLevel = y;
    lowLevel = y;
    connectCount = 0;
    Ocount = 0;
    int sixDecreaseO = 0 , sixDecreaseO_1 = 0 , sixDecreaseO_2 = 0;
    while ( 1 )
    {
        if ( board[x][j] == self )
        {
            connectCount ++;
        }
        if ( board[x][j] == enemy )
        {
            highLevel = j;
            break;
        }
        else
        {
            if ( board[x][j] == 0 )
            {
                if ( !IsValidPos(x,j+1) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    sixDecreaseO_1 = 1;
                }
                else
                {
                    if ( !IsValidPos(x,j+2) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        highLevel = j;
                        if ( j == y )
                        {
                            if ( board[x][j+1] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[x][j+1] == enemy )
                            {
                                highLevel = j + 1;
                            }
                            if ( board[x][j+1] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[x][j+1] == enemy )
                            {
                                highLevel = j + 1;
                                Ocount ++;
                            }
                            if ( board[x][j+1] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[x][j+1] == enemy )
                {
                    highLevel = j+1;
                    canGoFive = 1;
                    if ( j != y )
                    {
                        sixDecreaseO_1 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[x][j+1] == 0 && board[x][j+2] == enemy )
                {
                    highLevel = j;
                    if ( j == y )
                    {
                        sixDecreaseO_1 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[x][j+1] == 0 && board[x][j+2] == 0 )
                {
                    highLevel = j;
                    if ( j == y )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(x,j) )
                {
                    edgeBlock = 1;
                    highLevel = j;
                    break;
                }
            }
        }
        j++;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x][y+1] == 0 && ( board[x][y+2] == enemy || board[x][y+2] == 3 ) )
    {
        highLevel = y+2;
    }

    j = y;
    while ( 1 )
    {
        if ( board[x][j] == self )
        {
            connectCount ++;
        }
        if ( board[x][j] == enemy )
        {
            lowLevel = j;
            break;
        }
        else
        {
            if ( board[x][j] == 0 )
            {
                if ( !IsValidPos(x,j-1) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    sixDecreaseO_2 = 1;
                }
                else
                {
                    if ( !IsValidPos(x,j-2) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        lowLevel = j;
                        if ( j == y )
                        {
                            if ( board[x][j-1] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[x][j-1] == enemy )
                            {
                                lowLevel = j - 1;
                            }
                            if ( board[x][j-1] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[x][j-1] == enemy )
                            {
                                lowLevel = j - 1;
                                Ocount ++;
                            }
                            if ( board[x][j-1] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[x][j-1] == enemy )
                {
                    lowLevel = j-1;
                    canGoFive = 1;
                    if ( j != y )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[x][j-1] == 0 && board[x][j-2] == enemy )
                {
                    lowLevel = j;
                    if ( j == y )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[x][j-1] == 0 && board[x][j-2] == 0 )
                {
                    lowLevel = j;
                    if ( j == y )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(x,j) )
                {
                    edgeBlock = 1;
                    lowLevel = j;
                    break;
                }
            }
        }
        j--;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x][y-1] == 0 && ( board[x][y-2] == enemy || board[x][y-2] == 3 ) )
    {
        lowLevel = y-2;
    }
    sixDecreaseO = sixDecreaseO_1 + sixDecreaseO_2;
    if ( connectCount == 1 && board[x][lowLevel] == 0 && board[x][highLevel] == 0 )
    {
        if ( Ocount == 0 )
        {
            score += score_6;           //
        }
        else
        {
            if ( Ocount == 1 )
            {
                score += score_6_5;      //
            }
            else
            {
                if ( Ocount == 2)
                {
                    score += score_6_6; //
                }
            }
        }
    }
    if ( connectCount == 2 )
    {
        if ( board[x][highLevel] == 0 && board[x][lowLevel] == 0 )
        {
            if ( Ocount < 2 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount == 2 )
            {
                score += score_6_6;
            }
        }
        if ( ( board[x][highLevel] == 0 && board[x][lowLevel] != 0 && board[x][lowLevel] != self ) || ( board[x][highLevel] != 0 && board[x][highLevel] != self && board[x][lowLevel] == 0 ) )
        {
            score += score_6_6;
        }
        if ( board[x][lowLevel] != 0 && board[x][lowLevel] != self && board [x][highLevel] != 0 && board [x][highLevel] != self && highLevel - lowLevel >= 7 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_6_6;
            }
        }
    }
    if ( connectCount == 3 )
    {
        if ( board[x][lowLevel] == 0 && board[x][highLevel] == 0 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_2_9;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_3;
                }
            }
            if ( Ocount >= 1 )
            {
                if ( Ocount <= 2 )
                {
                    score += score_3_2;
                }
            }
        }
        if ( ( board[x][lowLevel] == 0 && board[x][highLevel] != 0 && board[x][highLevel] != self ) || ( board[x][lowLevel] != 0 && board[x][lowLevel] != self && board[x][highLevel] == 0 ) )
        {
            if ( Ocount <= 2 )
            {
                score += score_3_2;
            }
        }
        if ( board[x][lowLevel] != 0 && board[x][lowLevel] != self && board[x][highLevel] != 0 && board[x][highLevel] != self && highLevel - lowLevel >= 7 )
        {
            score += score_3_2;
        }
    }
    if ( connectCount >= 4 )
    {
        if ( step == 1 )
        {
            if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self && board[x][y-4] == self && ( board[x][y-5] == 0 || board[x][y+1] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self && board[x][y+4] == self && ( board[x][y+5] == 0 || board[x][y-1] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y-2] == 0 && board[x][y-3] == self && board[x][y-4] == self && board[x][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y+2] == 0 && board[x][y+3] == self && board[x][y+4] == self && board[x][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self && ( board[x][y+4] == 0 || board[x][y-2] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self && ( board[x][y-4] == 0 || board[x][y+2] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self && board[x][y-4] == 0 && board[x][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self && board[x][y+4] == 0 && board[x][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == 0 && board[x][y-4] == self && board[x][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == 0 && board[x][y+4] == self && board[x][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y+1] == self && board[x][y+2] == self && ( board[x][y-3] == 0 || board[x][y+3] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y+1] == 0 && board[x][y+2] == self && board[x][y+3] == self && board[x][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y-1] == 0 && board[x][y-2] == self && board[x][y-3] == self && board[x][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == 0 && board[x][y-2] == self && board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == 0 && board[x][y+2] == self && board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y+1] == 0 && board[x][y+2] == self && board[x][y+3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y+2] == self && board[x][y-1] == 0 && board[x][y-2] == self && board[x][y-3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y+1] == self && board[x][y+2] == 0 && board[x][y+3] == self && board[x][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y-1] == self && board[x][y-2] == 0 && board[x][y-3] == self && board[x][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y-2] == 0 && board[x][y-3] == self && board[x][y+1] == self && board[x][y+2] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y+2] == 0 && board[x][y+3] == self && board[x][y-1] == self && board[x][y-2] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y-1] == self && board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == 0 && board[x][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x][y+1] == self && board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == 0 && board[x][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
        }
        else
        {
            if ( board[x][y+1] == 0 && board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self && board[x][y-4] == self && board[x][y-5] == 0 && ( board[x][y-6] == enemy || board[x][y-6] == 3 ) )
            {
                score += score_3;
            }
            else
            {
                if ( board[x][y-1] == 0 && board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self && board[x][y+4] == self && board[x][y+5] == 0 && ( board[x][y+6] == enemy || board[x][y+6] == 3 ) )
                {
                    score += score_3;
                }
                else
                {
                    score += score_6_6;
                }
            }
        }

        if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self && board[x][y-4] == self && board[x][y-5] == self )
        {
            score += score_1;
        }
        if ( board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self && board[x][y+4] == self && board[x][y+5] == self )
        {
            score += score_1;
        }
        if ( board[x][y-1] == self && board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self && board[x][y+4] == self )
        {
            score += score_1;
        }
        if ( board[x][y+1] == self && board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self && board[x][y-4] == self )
        {
            score += score_1;
        }
        if ( board[x][y-1] == self && board[x][y-2] == self && board[x][y+1] == self && board[x][y+2] == self && board[x][y+3] == self )
        {
            score += score_1;
        }
        if ( board[x][y+1] == self && board[x][y+2] == self && board[x][y-1] == self && board[x][y-2] == self && board[x][y-3] == self )
        {
            score += score_1;
        }
    }
    return score;
}

// Set scores by right up direction.
int CMoveGenerator::set_by_direction4 ( char color , int x , int y , int step, char board[][GRID_NUM] )
{
    int self , enemy , highLevelx , lowLevelx , highLevely , lowLevely , i , j , connectCountUp = 0 , connectCountDn = 0 , OcountUp = 0 , OcountDn = 0 , score = 0 , middleCheck[2] = {0,0} , connectCount = 0 , defSuc = 0 , defVT = 0 , canGoFive = 0 , edgeBlock = 0 , Ocount = 0;

    if ( color == 1 )
    {
        self = 1;
        enemy = 2;
    }
    else
    {
        self = 2;
        enemy = 1;
    }

    OcountUp = 0;
    OcountDn = 0;
    connectCountUp = 0;
    connectCountDn = 0;
    connectCount = 0;
    highLevelx = x;
    highLevely = y;
    lowLevelx = x;
    lowLevely = y;

    i = x;
    j = y;
    while ( OcountUp < 3 )
    {
        i --;
        j --;
        if ( board[i][j] == enemy )
        {
            connectCountUp ++;
            middleCheck[0] = 1;
            continue;
        }
        else
        {
            if ( board[i][j] == self || !IsValidPos(i,j) )
            {
                highLevelx = i;
                highLevely = j;
                break;
            }
            else
            {
                if ( board[i][j] == 0 )
                {
                    if ( board[i-1][j-1] == 0 && board[i-2][j-2] != enemy )
                    {
                        highLevelx = i;
                        highLevely = j;
                        break;
                    }
                    else
                    {
                        OcountUp ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountUp == 3 )
    {
        highLevelx = i;
        highLevely = j;
        OcountUp = 2;
    }

    i = x;
    j = y;
    while ( OcountDn < 3 )
    {
        i ++;
        j ++;
        if ( board[i][j] == enemy )
        {
            connectCountDn ++;
            middleCheck[1] = 1;
            continue;
        }
        else
        {
            if ( board[i][j] == self || board[i][j] == 3 )
            {
                lowLevelx = i;
                lowLevely = j;
                break;
            }
            else
            {
                if ( board[i][j] == 0 )
                {
                    if ( board[i+1][j+1] == 0 && board[i+2][j+2] != enemy )
                    {
                        lowLevelx = i;
                        lowLevely = j;
                        break;
                    }
                    else
                    {
                        OcountDn ++;
                        continue;
                    }
                }
            }
        }
    }
    if ( OcountDn == 3 )
    {
        lowLevelx = i;
        lowLevely = j;
        OcountDn = 2;
    }
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( lowLevelx - x <= 6 || x - highLevelx <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountUp + OcountDn + 1;
    }
    if ( middleCheck[0] && !middleCheck[1] )
    {
        if ( ( board[highLevelx][highLevely] == 3 || board[highLevelx][highLevely] == self ) && ( board[lowLevelx][lowLevely] == 3 || board[lowLevelx][lowLevely] == self ) )
        {
            if ( lowLevelx - highLevelx <= 6 )
            {
                defSuc = 0;
            }
            else
            {
                defSuc = 1;
            }
        }
        else
        {
            if ( x - highLevelx <= 6 )
            {
                defSuc = 1;
            }
            else
            {
                defSuc = 0;
            }
        }
        Ocount = OcountUp;
    }
    if ( middleCheck[1] && !middleCheck[0] )
    {
        if ( lowLevelx - x <= 6 )
        {
            defSuc = 1;
        }
        else
        {
            defSuc = 0;
        }
        Ocount = OcountDn;
    }
    if ( ( board[highLevelx][highLevely] == 3 || board[highLevelx][highLevely] == self ) && ( board[lowLevelx][lowLevely] == 3 || board[lowLevelx][lowLevely] == self ) )
    {
        if ( lowLevelx - highLevelx <= 6 )
        {
            defSuc = 0;
        }
    }
    connectCount = connectCountUp + connectCountDn;
    if ( middleCheck[0] && middleCheck[1] )
    {
        if ( Ocount == 1 && board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] == 0 )
        {
            defVT = T;
        }
        else
        {
            defVT = V;
        }
    }
    else
    {
        if ( middleCheck[0] && !middleCheck[1] )
        {
            if ( OcountUp == 0 && board[highLevelx][highLevely] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountUp == 1 && board[x-1][y-1] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
        if ( !middleCheck[0] && middleCheck[1] )
        {
            if ( OcountDn == 0 && board[lowLevelx][lowLevely] == 0 )
            {
                defVT = T;
            }
            else
            {
                if ( OcountDn == 1 && board[x+1][y+1] == 0 )
                {
                    defVT = T;
                }
                else
                {
                    defVT = V;
                }
            }
        }
    }
    if ( defSuc )
    {
        if ( connectCount == 1 )
        {
            score += score_9;
        }
        if ( connectCount == 4 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                score += score_2;
                if ( step == 1 )
                {
                    if ( Ocount <= 2 )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
            else
            {
                if ( step == 2 )
                {
                    if ( defVT == V )
                    {
                        score += score_2;
                    }
                }
                else
                {
                    score += score_2;
                    if ( defVT == V )
                    {
                        m_dead_four_plus = 1;
                    }
                    add_new_pos_for_two(x,y);
                }
            }
        }
        if ( connectCount == 1 )
        {
            if ( board[x-1][y-1] == 0 && board[x-2][y-2] == enemy && board[x-3][y-3] == 0 && board[x-4][y-4] == 0 && board[x-5][y-5] == enemy && board[x-6][y-6] == 0 && board[x-7][y-7] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( board[x+1][y+1] == 0 && board[x+2][y+2] == enemy && board[x+3][y+3] == 0 && board[x+4][y+4] == 0 && board[x+5][y+5] == enemy && board[x+6][y+6] == 0 && board[x+7][y+7] == 0 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4_7; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
        }
        if ( connectCount == 2 )
        {
            if ( middleCheck[0] && middleCheck[1] && board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] == 0 && Ocount <= 2 )
            {
                if ( step == 2 && m_dead_four_plus == 1 )
                {
                    score += score_4; //90
                }
                else
                {
                    score += score_5;
                    if ( step == 1 )
                    {
                        add_new_pos_for_two(x,y);
                    }
                }
            }
            if ( middleCheck[0] && !middleCheck[1] && board[highLevelx][highLevely] == 0 )
            {
                if ( board[x-1][y-1] == 0 && board[x-2][y-2] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x-1][y-1] == enemy && board[x-2][y-2] == 0 && board[x-3][y-3] == 0 && board[x-4][y-4] == enemy && board[x-5][y-5] == 0 && ( board[x-6][y-6] == self || board[x-6][y-6] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x-1][y-1] == 0 && board[x-2][y-2] == enemy && board[x-3][y-3] == 0 && board[x-4][y-4] == enemy && board[x-5][y-5] == 0 && ( board[x-6][y-6] == self || board[x-6][y-6] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x-1][y-1] == enemy && ( board[x+1][y+1] == self || board[x+1][y+1] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( middleCheck[1] && !middleCheck[0] && board[lowLevelx][lowLevely] == 0 )
            {
                if ( board[x+1][y+1] == 0 && board[x+2][y+2] == 0 )
                {
                    score += 0;
                }
                else
                {
                    if ( board[x+1][y+1] == enemy && board[x+2][y+2] == 0 && board[x+3][y+3] == 0 && board[x+4][y+4] == enemy && board[x+5][y+5] == 0 && ( board[x+6][y+6] == self || board[x+6][y+6] == 3 ) )
                    {
                        score += 0;
                    }
                    else
                    {
                        if ( board[x+1][y+1] == 0 && board[x+2][y+2] == enemy && board[x+3][y+3] == 0 && board[x+4][y+4] == enemy && board[x+5][y+5] == 0 && ( board[x+6][y+6] == self || board[x+6][y+6] == 3 ) )
                        {
                            score += 0;
                        }
                        else
                        {
                            if ( board[x+1][y+1] == enemy && ( board[x-1][y-1] == self || board[x-1][y-1] == 3 ) )
                            {
                                score += 0;
                            }
                            else
                            {
                                if ( step == 2 && m_dead_four_plus == 1 )
                                {
                                    score += score_4; //90
                                }
                                else
                                {
                                    score += score_5;
                                    if ( step == 1 )
                                    {
                                        add_new_pos_for_two(x,y);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
        if ( connectCount == 3 )
        {
            if ( middleCheck[0] && middleCheck[1] )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 200
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //137
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( middleCheck[0] && !middleCheck[1] && ( board[x-1][y-1] == enemy || board[x-2][y-2] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

            if ( !middleCheck[0] && middleCheck[1] && ( board[x+1][y+1] == enemy || board[x+2][y+2] == enemy ) )
            {
                if ( defVT == T )
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_3_3; // 145
                    }
                    else
                    {
                        score += score_4;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
                else
                {
                    if ( step == 2 && m_dead_four_plus == 1 )
                    {
                        score += score_4_6; //80
                    }
                    else
                    {
                        score += score_5_5;
                        if ( step == 1 )
                        {
                            add_new_pos_for_two(x,y);
                        }
                    }
                }
            }

        }
        if ( connectCount == 5 )
        {
            score += score_2;
        }
        if ( connectCount == 6 )
        {
            score += score_2;
        }
        if ( connectCount == 7 )
        {
            score += score_2;
        }
        if ( connectCount == 8 )
        {
            score += score_2;
        }
        if ( connectCount == 9 )
        {
            score += score_2;
        }
        if ( connectCount == 10 )
        {
            score += score_2;
        }
    }
    else
    {
        score = 0;
    }
    i = x;
    j = y;
    highLevelx = x;
    lowLevelx = x;
    highLevely = y;
    lowLevely = y;
    connectCount = 0;
    Ocount = 0;
    int sixDecreaseO = 0 , sixDecreaseO_1 = 0 , sixDecreaseO_2 = 0;
    while ( 1 )
    {
        if ( board[i][j] == self )
        {
            connectCount ++;
        }
        if ( board[i][j] == enemy )
        {
            highLevelx = i;
            highLevely = j;
            break;
        }
        else
        {
            if ( board[i][j] == 0 )
            {
                if ( !IsValidPos(i-1,j-1) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    sixDecreaseO_1 = 1;
                }
                else
                {
                    if ( !IsValidPos(i-2,j-2) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        highLevelx = i;
                        highLevely = j;
                        if ( i == x )
                        {
                            if ( board[i-1][j-1] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[i-1][j-1] == enemy )
                            {
                                highLevelx = i - 1;
                                highLevely = j - 1;
                            }
                            if ( board[i-1][j-1] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[i-1][j-1] == enemy )
                            {
                                highLevelx = i - 1;
                                highLevely = j - 1;
                                Ocount ++;
                            }
                            if ( board[i-1][j-1] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[i-1][j-1] == enemy )
                {
                    highLevelx = i-1;
                    highLevely = j-1;
                    canGoFive = 1;
                    if ( i != x )
                    {
                        sixDecreaseO_1 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i-1][j-1] == 0 && board[i-2][j-2] == enemy )
                {
                    highLevelx = i;
                    highLevely = j;
                    if ( i == x )
                    {
                        sixDecreaseO_1 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i-1][j-1] == 0 && board[i-2][j-2] == 0 )
                {
                    highLevelx = i;
                    highLevely = j;
                    if ( i == x )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(i,j) )
                {
                    edgeBlock = 1;
                    highLevelx = i;
                    highLevely = j;
                    break;
                }
            }
        }
        i--;
        j--;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x-1][y-1] == 0 && ( board[x-2][y-2] == enemy || board[x-2][y-2] == 3 ) )
    {
        highLevelx = x-2;
        highLevely = y-2;
    }

    i = x;
    j = y;
    while ( 1 )
    {
        if ( board[i][j] == self )
        {
            connectCount ++;
        }
        if ( board[i][j] == enemy )
        {
            lowLevelx = i;
            lowLevely = j;
            break;
        }
        else
        {
            if ( board[i][j] == 0 )
            {
                if ( !IsValidPos(i+1,j+1) )
                {
                    edgeBlock = 1;
                    canGoFive = 1;
                    sixDecreaseO_2 = 1;
                }
                else
                {
                    if ( !IsValidPos(i+2,j+2) )
                    {
                        edgeBlock = 1;
                        canGoFive = 1;
                        lowLevelx = i;
                        lowLevely = j;
                        if ( i == x )
                        {
                            if ( board[i+1][j+1] == 0 )
                            {
                                sixDecreaseO_1 = 1;
                                Ocount ++;
                            }
                            if ( board[i+1][j+1] == enemy )
                            {
                                lowLevelx = i + 1;
                                lowLevely = j + 1;
                            }
                            if ( board[i+1][j+1] == self )
                            {
                                connectCount ++;
                            }
                        }
                        else
                        {
                            if ( board[i+1][j+1] == enemy )
                            {
                                lowLevelx = i + 1;
                                lowLevely = j + 1;
                                Ocount ++;
                            }
                            if ( board[i+1][j+1] == self )
                            {
                                connectCount ++;
                                Ocount ++;
                            }
                        }
                        break;
                    }
                }
                if ( board[i+1][j+1] == enemy )
                {
                    lowLevelx = i+1;
                    lowLevely = j+1;
                    canGoFive = 1;
                    if ( i != x )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i+1][j+1] == 0 && board[i+2][j+2] == enemy )
                {
                    lowLevelx = i;
                    lowLevely = j;
                    if ( i == x )
                    {
                        sixDecreaseO_2 = 1;
                        Ocount ++;
                    }
                    break;
                }
                if ( board[i+1][j+1] == 0 && board[i+2][j+2] == 0 )
                {
                    lowLevelx = i;
                    lowLevely = j;
                    if ( i == x )
                    {
                        Ocount ++;
                    }
                    break;
                }
                Ocount ++;
            }
            else
            {
                if ( !IsValidPos(i,j) )
                {
                    edgeBlock = 1;
                    lowLevelx = i;
                    lowLevely = j;
                    break;
                }
            }
        }
        i++;
        j++;
    }
    if ( Ocount >= 1 )
    {
        Ocount --;
    }
    if ( board[x+1][y+1] == 0 && ( board[x+2][y+2] == enemy || board[x+2][y+2] == 3 ) )
    {
        lowLevelx = x+2;
        lowLevely = y+2;
    }
    sixDecreaseO = sixDecreaseO_1 + sixDecreaseO_2 ;
    if ( connectCount == 1 && board[lowLevelx][lowLevely] == 0 && board[highLevelx][highLevely] == 0 )
    {
        if ( Ocount == 0 )
        {
            score += score_6;           //
        }
        else
        {
            if ( Ocount == 1 )
            {
                score += score_6_5;      //
            }
            else
            {
                if ( Ocount == 2)
                {
                    score += score_6_6; //
                }
            }
        }
    }
    if ( connectCount == 2 )
    {
        if ( board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] == 0 )
        {
            if ( Ocount < 2 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount == 2 )
            {
                score += score_6_6;
            }
        }
        if ( ( board[highLevelx][highLevely] == 0 && board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self ) || ( board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self && board[lowLevelx][lowLevely] == 0 ) )
        {
            score += score_6_6;
        }
        if ( board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self && board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self && lowLevelx - highLevelx >= 7 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_3;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_5;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_6_6;
            }
        }
    }
    if ( connectCount == 3 )
    {
        if ( board[lowLevelx][lowLevely] == 0 && board[highLevelx][highLevely] == 0 )
        {
            if ( Ocount == 0 )
            {
                if ( step == 1 )
                {
                    score += score_2_9;

                    add_new_pos_for_two(x,y);
                }
                else
                {
                    score += score_3;
                }
            }
            if ( Ocount >= 1 )
            {
                score += score_3_2;
            }
        }
        if ( ( board[lowLevelx][lowLevely] == 0 && board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self ) || ( board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self && board[highLevelx][highLevely] == 0 ) )
        {
            if ( Ocount <= 2 )
            {
                score += score_3_2;
            }
        }
        if ( board[lowLevelx][lowLevely] != 0 && board[lowLevelx][lowLevely] != self && board[highLevelx][highLevely] != 0 && board[highLevelx][highLevely] != self && lowLevelx - highLevelx >= 7 )
        {
            if ( Ocount <= 2 )
            {
                score += score_6_6;
            }
        }
    }
    if ( connectCount >= 4 )
    {
        if ( step == 1 )
        {
            if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self && board[x-4][y-4] == self && ( board[x-5][y-5] == 0 || board[x+1][y+1] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self && board[x+4][y+4] == self && ( board[x+5][y+5] == 0 || board[x-1][y-1] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x-2][y-2] == 0 && board[x-3][y-3] == self && board[x-4][y-4] == self && board[x-5][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x+2][y+2] == 0 && board[x+3][y+3] == self && board[x+4][y+4] == self && board[x+5][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self && ( board[x+4][y+4] == 0 || board[x-2][y-2] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self && ( board[x-4][y-4] == 0 || board[x+2][y+2] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self && board[x-4][y-4] == 0 && board[x-5][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self && board[x+4][y+4] == 0 && board[x+5][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == 0 && board[x-4][y-4] == self && board[x-5][y-5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == 0 && board[x+4][y+4] == self && board[x+5][y+5] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x+1][y+1] == self && board[x+2][y+2] == self && ( board[x-3][y-3] == 0 || board[x+3][y+3] == 0 ) )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }

            if ( board[x-1][y-1] == self && board[x+1][y+1] == 0 && board[x+2][y+2] == self && board[x+3][y+3] == self && board[x+4][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x-1][y-1] == 0 && board[x-2][y-2] == self && board[x-3][y-3] == self && board[x-4][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == 0 && board[x-2][y-2] == self && board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == 0 && board[x+2][y+2] == self && board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x+1][y+1] == 0 && board[x+2][y+2] == self && board[x+3][y+3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x+2][y+2] == self && board[x-1][y-1] == 0 && board[x-2][y-2] == self && board[x-3][y-3] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x+1][y+1] == self && board[x+2][y+2] == 0 && board[x+3][y+3] == self && board[x+4][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x-1][y-1] == self && board[x-2][y-2] == 0 && board[x-3][y-3] == self && board[x-4][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x-2][y-2] == 0 && board[x-3][y-3] == self && board[x+1][y+1] == self && board[x+2][y+2] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x+2][y+2] == 0 && board[x+3][y+3] == self && board[x-1][y-1] == self && board[x-2][y-2] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x-1][y-1] == self && board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == 0 && board[x+4][y+4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
            if ( board[x+1][y+1] == self && board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == 0 && board[x-4][y-4] == self )
            {
                score += score_1;
                add_new_pos_for_two(x,y);
            }
        }
        else
        {
            if ( board[x+1][y+1] == 0 && board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self && board[x-4][y-4] == self && board[x-5][y-5] == 0 && ( board[x-6][y-6] == enemy || board[x-6][y-6] == 3 ) )
            {
                score += score_3;
            }
            else
            {
                if ( board[x-1][y-1] == 0 && board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self && board[x+4][y+4] == self && board[x+5][y+5] == 0 && ( board[x+6][y+6] == enemy || board[x+6][y+6] == 3 ) )
                {
                    score += score_3;
                }
                else
                {
                    score += score_6_6;
                }
            }
        }

        if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self && board[x-4][y-4] == self && board[x-5][y-5] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self && board[x+4][y+4] == self && board[x+5][y+5] == self )
        {
            score += score_1;
        }
        if ( board[x-1][y-1] == self && board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self && board[x+4][y+4] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y+1] == self && board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self && board[x-4][y-4] == self )
        {
            score += score_1;
        }
        if ( board[x-1][y-1] == self && board[x-2][y-2] == self && board[x+1][y+1] == self && board[x+2][y+2] == self && board[x+3][y+3] == self )
        {
            score += score_1;
        }
        if ( board[x+1][y+1] == self && board[x+2][y+2] == self && board[x-1][y-1] == self && board[x-2][y-2] == self && board[x-3][y-3] == self )
        {
            score += score_1;
        }
    }
    return score;
}


// ======================= search_engine.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


CSearchEngine::CSearchEngine()
{}

void CSearchEngine::before_search(char board[][GRID_NUM], char color, int alphabeta_depth){

    memcpy(m_board, board, sizeof(m_board));
    m_chess_type = color;
    m_alphabeta_depth = alphabeta_depth;
    // For debug.
    m_total_nodes = 0;
    m_move_gernerator.m_time_get_moves = 0;
    m_move_gernerator.m_time_set_score = 0;
    m_move_gernerator.m_time_test = 0;
    m_evaluator.m_time_evalution = 0;

}

static int move_cmp(const void* move1,const void* move2)
{
    return (int)(((move_t*)move2)->score - ((move_t*)move1)->score);
}


double CSearchEngine::alpha_beta_search(int depth,double alpha,double beta,char ourColor ,move_t* bestMove,move_t* preMove)
{
    
    if (cloudict_time_up()) {
        return m_evaluator.evaluation(m_chess_type, ourColor, m_board);
    }
move_t moveList[NUMOFONE * NUMOFTWO * 2];
    move_t tempBest;

    double val = 0;
    int n = 0;
    int beg, end;

    m_total_nodes++;

    if (is_win_by_premove(m_board, preMove))
    {
        if (ourColor == m_chess_type)
        {
            // Opponent wins.
            return 0;
        } else
        {
            // Self wins.
            return MININT + 1;
        }
    }
    if (depth == 0)
    {
        if (m_alphabeta_depth & 1) 
            return -m_evaluator.evaluation(m_chess_type,ourColor,m_board);
        else return m_evaluator.evaluation(m_chess_type,ourColor,m_board);
    }

    // Get move list.
    n = m_move_gernerator.get_move_list(ourColor,moveList,m_board);
    if (n < 1)
    {
        bestMove->positions[0].x = 10;
        bestMove->positions[0].y = 10;
        bestMove->positions[1].x = 10;
        bestMove->positions[1].y = 10;
        return 0;
    }
    else
    {
        bestMove->positions[0].x = moveList[0].positions[0].x;
        bestMove->positions[0].y = moveList[0].positions[0].y;
        bestMove->positions[1].x = moveList[0].positions[1].x;
        bestMove->positions[1].y = moveList[0].positions[1].y;
    }
    qsort(moveList,n,sizeof(move_t),move_cmp);
    beg = 0;
    end = n;

    double pvs_beta = beta;

    for(int i = beg ; i< end ; i++)
    {

        if (cloudict_time_up()) break;
m_board[moveList[i].positions[0].x][moveList[i].positions[0].y] = ourColor;
        m_board[moveList[i].positions[1].x][moveList[i].positions[1].y] = ourColor;

        // Alpha beta search.
        val = -alpha_beta_search(depth-1,-beta,-alpha,ourColor^(char)3,&tempBest,&moveList[i]);
        moveList[i].score = val;

        // Unmake the move.
        m_board[moveList[i].positions[0].x][moveList[i].positions[0].y] = NOSTONE;
        m_board[moveList[i].positions[1].x][moveList[i].positions[1].y] = NOSTONE;

        // Alpha beta prune.
        if (val >= beta)
        {
            return val;
        }

        pvs_beta = alpha + 1;

        if (val > alpha)
        {
            alpha = val;
            bestMove->positions[0].x = moveList[i].positions[0].x;
            bestMove->positions[0].y = moveList[i].positions[0].y;
            bestMove->positions[1].x = moveList[i].positions[1].x;
            bestMove->positions[1].y = moveList[i].positions[1].y;
            bestMove->score = alpha;
        }
    }

    return alpha;
}


// ======================= vcf_search.cc =======================

/*
 * Copyright (c) 2008-2013 Hao Cui <Hao.Cui@Tufts.edu>,
 *                         Liang Li <liliang010@gmail.com>,
 *                         Ruijian Wang <jeoygin@gmail.com>,
 *                         Siran Lin <linsiran@gmail.com>.
 *                         All rights reserved.
 *
 * This program is a free software; you can redistribute it and/or modify
 * it under the terms of the BSD license. See LICENSE.txt for details.
 *
 * Date: 2013/11/01
 *
 */


CVCFSearch::CVCFSearch() {
    m_dx[0] = -1;
    m_dx[1] = -1;
    m_dx[2] = 0;
    m_dx[3] = 1;

    m_dy[0] = 0;
    m_dy[1] = -1;
    m_dy[2] = -1;
    m_dy[3] = -1;
    // m_dy[4]={0,-1,-1,-1};
    // m_dx[4]={-1,-1,0,1};        // Directions.
}

int CVCFSearch::init() {
    return m_dfa.dfa_init();
}

void CVCFSearch::init_game() {
    m_has_win = 0;
}

void CVCFSearch::before_search(char board[][GRID_NUM], char color){
    memcpy(m_board, board, sizeof(m_board));
    m_chess_type = color;
    m_vcf_node = 0;
}

bool CVCFSearch::vcf_judge(move_t * preMove)
{
    char Color=m_board[preMove->positions[0].x][preMove->positions[0].y];
    if (is_attack(m_board,Color,preMove)>=2)
    {
        return true;
    }
    return false;
}

int CVCFSearch::is_attack(char board[GRID_NUM][GRID_NUM],char Color, move_t * Move)
{
    int x, y, newx, newy;
    int count;
    int i, d, j;
    char tmpboard[GRID_NUM][GRID_NUM];
    char tmpuse[GRID_NUM][GRID_NUM][4];
    int max, min, ans=0;

    ans=is_dlb_attack(board,Color,Move);
    if (ans>2)                                      // More than two threats.
    {
        return ans;
    }

    memcpy(tmpboard,board,sizeof(tmpboard));
    memcpy(tmpuse,m_vcf_use,sizeof(tmpuse));

    for ( i = 0; i <= 1; i++ )
    {
        x = Move->positions[i].x;                   // Pre move.
        y = Move->positions[i].y;
        for ( d = 0; d < 4; d++  )                  // For four directions.
        {
            // Get our zones in each direction.
            newx = x;
            newy = y;
            for ( j = 1; j < 6 ; j++ )
            {
                newx += m_dx[d];
                newy += m_dy[d];
                if ( tmpboard[newx][newy]!=BORDER && tmpboard[newx][newy] != (Color^3) )
                {
                    if (tmpboard[newx][newy]==NOSTONE && m_vcf_mark[newx][newy])
                    {
                        break;
                    }
                } else
                {
                    break;
                }
            }
            max = j - 1;    // Max point.

            newx = x;
            newy = y;
            for ( j = 1; j < 6 ; j++)
            {
                newx -= m_dx[d];
                newy -= m_dy[d];
                if ( tmpboard[newx][newy]!=BORDER && tmpboard[newx][newy] != (Color^3) )
                {
                    if (tmpboard[newx][newy]==NOSTONE&& m_vcf_mark[newx][newy])
                    {
                        break;
                    }
                } else
                {
                    break;
                }
            }
            min = 1 - j;

            if ( max - min + 1 < 6 )    // Less than six.
            {
                continue;
            }
            j = max - 5;                // Calculate how many points are ours, in the connected six.
            newx = x + m_dx[d] * j;
            newy = y + m_dy[d] * j;
            count=0;
            for (; j <= max; j++)
            {
                if (tmpboard[newx][newy]==Color)
                {
                    count++;
                }
                //use[newx][newy][d]=1;
                //mark[newx][newy]=1;
                newx += m_dx[d];
                newy += m_dy[d];
            }

            if ( count >= 4 )           // One threat formed.
            {
                ans++;
                for (j=0;j<6;j++)
                {
                    newx-=m_dx[d];
                    newy-=m_dy[d];
                    m_vcf_use[newx][newy][d]=i+1;
                    m_vcf_mark[newx][newy]=1;
                }
                continue;
            }

            j = max - 6;                // Next connected six points.
            newx = x + m_dx[d] * j;
            newy = y + m_dy[d] * j;
            for (; j >= min; j--)
            {

                if (tmpboard[newx][newy]==Color)
                {
                    count++;
                }
                if (tmpboard[newx+m_dx[d]*6][newy+m_dy[d]*6])
                {
                    count--;
                }
                //use[newx][newy][d]=1;
                //mark[newx][newy]=1;
                /*if (tmpuse[newx+dx[d]*6][newy+dy[d]*6][d]==0)
                {
                use[newx+dx[d]*6][newy+dy[d]*6][d]=0;
                mark[newx+dx[d]*6][newy+dy[d]*6]=0;
                }*/

                if (count>=4)
                {
                    ans++;
                    for (int k=0;k<6;k++)
                    {
                        m_vcf_use[newx][newy][d]=i+1;
                        m_vcf_mark[newx][newy]=1;
                        newx+=m_dx[d];
                        newy+=m_dy[d];
                    }
                    break;
                }
                newx-=m_dx[d];
                newy-=m_dy[d];
            }

            if (count<4)
            {
                newx=x+m_dx[d]*min;
                newy=y+m_dy[d]*min;

            }

        }
    }
    return ans;
}

int CVCFSearch::is_dlb_attack(char board[][GRID_NUM],char Color, move_t * Move)
{
    int x, y, newx, newy;
    int count;
    int i, d, j;
    char tmpboard[GRID_NUM][GRID_NUM];
    int min, max;

    memset(m_vcf_use,0,sizeof(m_vcf_use));
    memset(m_vcf_mark,0,sizeof(m_vcf_mark));
    memcpy(tmpboard,board,sizeof(tmpboard));        // Create a tmp board.

    for ( i = 0; i <= 1; i++ )
    {
        x = Move->positions[i].x;                   // Pre move.
        y = Move->positions[i].y;
        for ( d = 0; d < 4; d++  )                  // For four direction.
        {
            count = 1;                              // Connected points.
            for ( j = 1; count < 6 ; j++ )
            {
                newx = x + m_dx[d] * j;
                newy = y + m_dy[d] * j;
                if ( tmpboard[newx][newy] == Color) // Connected.
                {
                    count++;
                }
                else                                // Not connected, exit.
                {
                    break;
                }
            }
            max = j;
            for ( j = 1; count < 6 ; j++)
            {
                newx = x - m_dx[d] * j;
                newy = y - m_dy[d] * j;
                if ( tmpboard[newx][newy] == Color) // Connected.
                {
                    count++;
                }
                else                                // Not connected, exit.
                {
                    break;
                }
            }
            min = - j;
            if ( count >= 6 )                       // Connected six points.
            {
                return 100;
            }

            if (count<4)
            {
                continue;
            }

            // Connected five or four points.
            int rightx = x + m_dx[d] * max;
            int righty = y + m_dy[d] * max;
            int leftx,lefty;
            if (tmpboard[rightx][righty] == NOSTONE) // NOSTONE.
            {
                leftx = x + m_dx[d] * min;
                lefty = y + m_dy[d] * min;
                if ( tmpboard[leftx][lefty] == NOSTONE)
                {
                    if (count==4&&(tmpboard[leftx-m_dx[d]][lefty-m_dy[d]]==BORDER||tmpboard[leftx-m_dx[d]][lefty-m_dy[d]]==(Color^3))
                        ||(tmpboard[rightx+m_dx[d]][righty+m_dy[d]]==BORDER||tmpboard[rightx+m_dx[d]][righty+m_dy[d]]==(Color^3)))
                    {
                        continue;
                    }
                    if (count==4)
                    {
                        m_vcf_mark[leftx-m_dx[d]][lefty-m_dy[d]]=1;
                        m_vcf_mark[rightx+m_dx[d]][righty+m_dy[d]]=1;
                    }
                    //Ļ
                    for (j=min;j<=max;j++)
                    {
                        newx = x + m_dx[d] * j;
                        newy = y + m_dy[d] * j;
                        m_vcf_use[newx][newy][d]=i+1;
                        m_vcf_mark[newx][newy]=1;
                    }
                    return 2;
                }
            }

        }
    }
    return 0;
}

int CVCFSearch::is_three(char position[GRID_NUM][GRID_NUM],char Color, pos_t * Pos)
{
    int x, y, newx, newy;
    int count;
    int  d, j;
    int min, max;

    char tmpboard[GRID_NUM][GRID_NUM];
    memcpy(tmpboard,position,sizeof(tmpboard));

    x=Pos->x;
    y=Pos->y;
    for ( d = 0; d < 4; d++  )            // Four direction.
    {
        count = 1;
        for ( j = 1; j < 6 ; j++ )
        {
            newx = x + m_dx[d] * j;
            newy = y + m_dy[d] * j;
            if ( tmpboard[newx][newy] == Color )
            {
                count++;
            }
            else if(tmpboard[newx][newy] != NOSTONE)
            {
                break;
            }
        }
        max = j - 1;
        for ( j = 1; j < 6 ; j++)
        {
            newx = x - m_dx[d] * j;
            newy = y - m_dy[d] * j;
            if ( tmpboard[newx][newy] == Color)             // Connected.
            {
                count++;
            }
            else if(tmpboard[newx][newy] != NOSTONE)        // Not Connected, exit.
            {
                break;
            }
        }
        min = 1 - j;

        if ( max - min + 1 < 6 )                            // Less than six.
        {
            continue;
        }
        if (count<3)
        {
            continue;
        }
        j = max - 5;                                        // Calculate how many points of ours.
        newx = x + m_dx[d] * j;
        newy = y + m_dy[d] * j;
        count=0;
        for (; j <= max; j++)
        {
            if (tmpboard[newx][newy]==Color)
            {
                count++;
            }
            newx += m_dx[d];
            newy += m_dy[d];
        }

        if ( count >= 3 )                                   // More than three.
        {
            return 1;
        }

        j = max - 6;                                        // Move to next zone.
        newx = x + m_dx[d] * j;
        newy = y + m_dy[d] * j;
        for (; j >= min; j--)
        {

            if (tmpboard[newx][newy]==Color)
            {
                count++;
            }
            if (tmpboard[newx+m_dx[d]*6][newy+m_dy[d]*6]==Color)
            {
                count--;
            }
            if (count>=3)
            {
                return 1;
            }
            newx-=m_dx[d];
            newy-=m_dy[d];
        }
    }
    return 0;
}

int CVCFSearch::vcf_get_move_list( char ourColor,char a_d, pos_t * canUse, int n_Pos, move_t * moveList, move_t * preMove)
{
    int n_MoveList=0, Count=0;
    int i, j, d, max, min;

    int x, y, newx, newy;


    char tmpboard[GRID_NUM][GRID_NUM];
    memcpy(tmpboard,m_board,sizeof(tmpboard));              // Create a tmp board.

    if (a_d==1)                                             // Attack.
    {
        // Siran Lin's
        n_MoveList=m_dfa.pattern_match(ourColor,moveList, tmpboard);
        for (i=0,j=0;i<n_MoveList;i++)
        {
            move_t tmpMove, orgMove;
            int tmpCount;
            orgMove=moveList[i];
            tmpboard[moveList[i].positions[0].x][moveList[i].positions[0].y]=ourColor;
            tmpboard[moveList[i].positions[1].x][moveList[i].positions[1].y]=ourColor;
            tmpMove.positions[0] = moveList[i].positions[1];
            tmpMove.positions[1] = moveList[i].positions[0];
            Count=is_attack(tmpboard,ourColor,moveList+i);
            if ((tmpCount=is_attack(tmpboard,ourColor,&tmpMove)) > Count)
            {
                Count = tmpCount;
                moveList[i] = tmpMove;
            }

            if (Count>=2)
            {
                moveList[j]=moveList[i];
                moveList[j++].score=Count;
                if (Count>=50)
                {
                    tmpMove=moveList[j-1];
                    moveList[j-1]=moveList[0];
                    moveList[0]=tmpMove;
                }
            }

            tmpboard[orgMove.positions[0].x][orgMove.positions[0].y]=NOSTONE;
            tmpboard[orgMove.positions[1].x][orgMove.positions[1].y]=NOSTONE;
        }
        n_MoveList=j;
        for (i=0,j=0;i<n_MoveList;i++)
        {
            int k;
            for(k=i+1;k<n_MoveList;k++)
            {
                if(moveList[i].positions[0].x==moveList[k].positions[0].x&&moveList[i].positions[0].y==moveList[k].positions[0].y&&
                    moveList[i].positions[1].x==moveList[k].positions[1].x&&moveList[i].positions[1].y==moveList[k].positions[1].y ||
                    moveList[i].positions[0].x==moveList[k].positions[1].x&&moveList[i].positions[0].y==moveList[k].positions[1].y&&
                    moveList[i].positions[1].x==moveList[k].positions[0].x&&moveList[i].positions[1].y==moveList[k].positions[0].y)
                {
                    break;
                }
            }
            if (k>=n_MoveList)
            {
                moveList[j++]=moveList[i];
            }
        }
        n_MoveList=j;

    }
    else                            // Defence.
    {
        if (is_attack(tmpboard,ourColor^3,preMove)!=2)
        {
            return 0;
        }

        pos_t Pos[2][10];
        int nPos[2];
        nPos[0]=nPos[1]=0;
        int dir=0;
        for ( i = 0; i <= 1; i++ )
        {
            x = preMove->positions[i].x;                        // Pre move.
            y = preMove->positions[i].y;
            for ( d = 0; d < 4; d++  )                          // Four directions.
            {

                if (!m_vcf_use[x][y][d])                        // Can't be used, skip.
                {
                    continue;
                }
                if (dir>=2)
                {
                    break;
                }
                // Find the scored points in current direction.
                int n_Nostone=0;
                Count=0;
                for ( j = 1; ; j++ )
                {
                    newx = x + m_dx[d] * j;
                    newy = y + m_dy[d] * j;
                    if ( m_vcf_use[newx][newy][d] )             // Can be used points.
                    {
                        if (tmpboard[newx][newy]==NOSTONE)
                        {
                            n_Nostone++;
                            if (m_vcf_use[newx][newy][d]==i+1)
                            {
                                Pos[dir][nPos[dir]].x=newx;
                                Pos[dir][nPos[dir]++].y=newy;
                            }
                            else
                            {
                                break;
                            }
                        }

                    }
                    else
                    {
                        break;
                    }
                }
                newx-=m_dx[d];
                newy-=m_dy[d];
                if (tmpboard[newx][newy]==NOSTONE)//&& tmpboard[newx+dx[d]][newy+dy[d]]!=BORDER
                    //&&tmpboard[newx+dx[d]][newy+dy[d]]!=ourColor)
                {
                    Count++;
                }
                max=j-1;                                // Max point.

                for ( j = 1; ; j++ )
                {
                    newx = x - m_dx[d] * j;
                    newy = y - m_dy[d] * j;
                    if ( m_vcf_use[newx][newy][d] )     // Can be used.
                    {
                        if (tmpboard[newx][newy]==NOSTONE)
                        {
                            n_Nostone++;
                            if (m_vcf_use[newx][newy][d]==i+1)
                            {
                                Pos[dir][nPos[dir]].x=newx;
                                Pos[dir][nPos[dir]++].y=newy;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else        // Exit.
                    {
                        break;
                    }
                }

                newx+=m_dx[d];
                newy+=m_dy[d];
                if ( tmpboard[newx][newy]==NOSTONE)// && tmpboard[newx-dx[d]][newy-dy[d]]!=BORDER
                    //&&tmpboard[newx-dx[d]][newy-dy[d]]!=ourColor)
                {
                    Count++;
                }
                min=1-j;                                // Min points.

                if (max-min+1<6)
                {
                    continue;
                }

                dir++;


                if (Count==2&&n_Nostone==2)             // two threats.
                {
                    if (max-min==5)
                    {
                        newx=x+m_dx[d]*(max+1);
                        newy=y+m_dy[d]*(max+1);
                        if (tmpboard[newx][newy]==BORDER||tmpboard[newx][newy]==ourColor)
                        {
                            continue;
                        }
                        newx=x+m_dx[d]*(min-1);
                        newy=y+m_dy[d]*(min-1);
                        if (tmpboard[newx][newy]==BORDER||tmpboard[newx][newy]==ourColor)
                        {
                            continue;
                        }
                    }
                    n_MoveList=0;
                    newx=x+m_dx[d]*max;
                    newy=y+m_dy[d]*max;
                    moveList[n_MoveList].positions[0].x=newx;
                    moveList[n_MoveList].positions[0].y=newy;
                    newx=x+m_dx[d]*min;
                    newy=y+m_dy[d]*min;
                    moveList[n_MoveList].positions[1].x=newx;
                    moveList[n_MoveList].positions[1].y=newy;
                    n_MoveList++;
                    newx=x+m_dx[d]*(max+1);
                    newy=y+m_dy[d]*(max+1);
                    if ( tmpboard[newx][newy]==NOSTONE)
                    {
                        moveList[n_MoveList].positions[0].x=newx;
                        moveList[n_MoveList].positions[0].y=newy;
                        moveList[n_MoveList].positions[1]=moveList[0].positions[1];
                        n_MoveList++;
                    }
                    newx=x+m_dx[d]*(min-1);
                    newy=y+m_dy[d]*(min-1);
                    if ( tmpboard[newx][newy]==NOSTONE)
                    {
                        moveList[n_MoveList].positions[0].x=newx;
                        moveList[n_MoveList].positions[0].y=newy;
                        moveList[n_MoveList].positions[1]=moveList[0].positions[0];
                        n_MoveList++;
                    }
                    return n_MoveList;
                }

            }
        }

        n_MoveList=0;
        for (i=0;i<nPos[0];i++)
        {
            for (j=0;j<nPos[1];j++)                     // Generate the moves.
            {
                if (Pos[0][i].x!=Pos[1][j].x||Pos[0][i].y!=Pos[1][j].y)
                {
                    moveList[n_MoveList].positions[0]=Pos[0][i];
                    moveList[n_MoveList].positions[1]=Pos[1][j];
                    n_MoveList++;
                }

            }
        }
    }
    return n_MoveList;
}

inline int CVCFSearch::vcf_abs(int a)
{
    return a >= 0 ? a : -a;
}

inline int CVCFSearch::dist(pos_t p1, pos_t p2, pos_t pt)
{
    return (vcf_abs(p1.x-pt.x)+vcf_abs(p1.y-pt.y)+vcf_abs(p2.x-pt.x)+vcf_abs(p2.y-pt.y));
}

inline int CVCFSearch::vcf_min(int a, int b)
{
    return a <= b ? a : b;
}

static int cmp(const void * a, const void * b)
{
    ListNode aa=*(ListNode*)a, bb=*(ListNode*)b;
    if (aa.score!=bb.score)
    {
        return bb.score-aa.score;
    }

    return aa.dist-bb.dist;

}

void CVCFSearch::sort(move_t * moveList, int n_moveList, move_t * preMove)
{
    int i;

    for (i=0;i<n_moveList;i++)
    {
        m_tmp_move_list[i]=moveList[i];
        m_list_node[i].dist=vcf_min(dist(moveList[i].positions[0],moveList[i].positions[1],preMove->positions[0]),
                             dist(moveList[i].positions[0],moveList[i].positions[1],preMove->positions[1]));
        m_list_node[i].score= (int)moveList[i].score;
        m_list_node[i].pos=i;
    }

    qsort(m_list_node,n_moveList,sizeof(ListNode),cmp);
    for (i=0;i<n_moveList;i++)
    {
        moveList[i]=m_tmp_move_list[m_list_node[i].pos];
    }

}

unsigned long CVCFSearch::vcf_hash_board(char board[GRID_NUM][GRID_NUM])
{
    unsigned long ans=0;
    int i, j;
    for (i=1;i<=BOARD_SIZE;i++)
    {
        for (j=1;j<=BOARD_SIZE;j++)
        {
            ans=(ans<<4)+board[i][j];
            unsigned long g=ans & 0xf0000000L;
            if (g)
            {
                ans^=g>>24;
            }
            ans &= ~g;
        }
    }
    return ans;
}

int CVCFSearch::vcf_hash_check(HashNode node)
{
    int i, t;
    int dep=node.dep;
    unsigned int hash=node.hash;
    int p=hash%HASHSIZE;
    char tmpboard[GRID_NUM][GRID_NUM];
    memcpy(tmpboard,m_org_board,sizeof(tmpboard));

    tmpboard[node.move.positions[0].x][node.move.positions[0].y]=node.color;
    tmpboard[node.move.positions[1].x][node.move.positions[1].y]=node.color;
    i=node.pre;
    while (i>=0)
    {
        tmpboard[m_hash_que[i].move.positions[0].x][m_hash_que[i].move.positions[0].y]=m_hash_que[i].color;
        tmpboard[m_hash_que[i].move.positions[1].x][m_hash_que[i].move.positions[1].y]=m_hash_que[i].color;
        i=m_hash_que[i].pre;
    }

    for (int h = m_hash_head[dep][p]; h >= 0; h = m_hash_next[h])
    {
        if (m_hash_que[h].hash==hash)
        {
            t=h;
            while (t>=0)
            {
                if (tmpboard[m_hash_que[t].move.positions[0].x][m_hash_que[t].move.positions[0].y]!=m_hash_que[t].color||
                    tmpboard[m_hash_que[t].move.positions[1].x][m_hash_que[t].move.positions[1].y]!=m_hash_que[t].color)
                {
                    break;
                }
                t=m_hash_que[t].pre;
            }
            if (t<0)
            {
                return h;
            }
        }
    }
    return -1;
}

bool CVCFSearch::anti_vcf_search(int depth,char ourColor,move_t * bestMove,move_t * preMove, int preNode, int prePos)
{
    
    if (cloudict_time_up()) return false;
int Color = ourColor;
    int i,j, t;
    int n_pos=0, n_moveList;
    pos_t canUse[GRID_NUM*GRID_NUM];                        // Points that can be used.
    move_t * moveList = m_vcf_move_list[depth];
    bool flag;
    int NodeID;
    int CurPos=m_vcf_node++;
    char tmpboard[GRID_NUM][GRID_NUM];

    memcpy(tmpboard,m_board,sizeof(tmpboard));

    if (depth==0)
    {
        memcpy(m_org_board,tmpboard,sizeof(tmpboard));
        memset(m_hash_head, -1, sizeof(m_hash_head));
        memset(m_hash_next, -1, sizeof(m_hash_next));
        m_vcf_total_node=0;
    }

    m_hash_que[CurPos].move=*preMove;
    m_hash_que[CurPos].pre=prePos;
    m_hash_que[CurPos].hash=vcf_hash_board(tmpboard);
    m_hash_que[CurPos].dep=depth;
    m_hash_que[CurPos].color=Color;
    if ((t=vcf_hash_check(m_hash_que[CurPos]))!=-1)
    {
        return m_hash_que[t].res;
    }

    if (depth>=ANTIVCFDEPTH)                        // Fail, if exceed the depth limitation.
    {
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;
    }

    if (is_win_by_premove(m_board, preMove))        // If won by some one, return.
    {
        m_hash_que[CurPos].res=(Color!=(m_chess_type));
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return Color!=(m_chess_type);
    }

    if (Color==((m_chess_type)^3) && is_attack(m_board,Color^3,preMove))
    {
        n_moveList=vcf_get_move_list(Color,1,canUse,n_pos,moveList, preMove);   // Get the move list.

        sort(moveList,n_moveList,preMove);

        for (i=0;i<n_moveList;i++)
        {
            if (moveList[i].score>=50)
            {
                NodeID=++m_vcf_total_node;
                if (depth==0)
                {
                    *bestMove=moveList[i];
                    m_vcf_now_pos=NodeID;
                }

                m_hash_que[CurPos].res=true;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }

                return true;
            }
            make_move(m_board,moveList+i, Color);
            if (is_attack(m_board,Color^3,preMove))
            {
                unmake_move(m_board, moveList+i);
                continue;
            }
            NodeID=++m_vcf_total_node;
            flag = anti_vcf_search(depth+1,ourColor^3,bestMove,moveList+i,NodeID,CurPos); //һVCFĽ
            unmake_move(m_board, moveList+i);
            if (flag)                                       // If VCF win, return.
            {
                if (depth==0)
                {
                    *bestMove=moveList[i];
                    m_vcf_now_pos=NodeID;
                }

                m_hash_que[CurPos].res=true;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }

                return true;
            }
            m_vcf_total_node=NodeID-1;
        }
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;                                       // Failed.
    }

    if (Color==(m_chess_type)&& is_attack(m_board,Color^3,preMove)>2)
    {
        m_hash_que[CurPos].res=true;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return true;
    }



    if ( Color==((m_chess_type)^3) )
    {
        n_moveList=vcf_get_move_list(Color,1,canUse,n_pos,moveList, preMove); //з

        sort(moveList,n_moveList,preMove);

        for (i=0;i<n_moveList;i++)
        {
            NodeID=++m_vcf_total_node;
            make_move(m_board, moveList+i, Color);
            flag = anti_vcf_search(depth+1,ourColor^3,bestMove,moveList+i,NodeID,CurPos);
            unmake_move(m_board, moveList+i);
            if (flag)
            {
                if (depth==0)
                {

                    *bestMove=moveList[i];
                    m_vcf_now_pos=NodeID;
                }
                m_hash_que[CurPos].res=true;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
                return true;
            }
            m_vcf_total_node=NodeID-1;
        }
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;
    }
    else
    {

        n_moveList=vcf_get_move_list(Color,0,canUse,n_pos,moveList, preMove);
        j=m_vcf_total_node;
        for (i=0;i<n_moveList;i++)
        {
            NodeID=++m_vcf_total_node;
            make_move(m_board, moveList+i, Color);
            flag = anti_vcf_search(depth+1,ourColor^3,bestMove,moveList+i,NodeID,CurPos);
            unmake_move(m_board, moveList+i);
            if ( !flag )
            {
                m_vcf_total_node=j;
                m_hash_que[CurPos].res=false;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
                return false;
            }

        }
        m_hash_que[CurPos].res=true;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return true;
    }
}

bool CVCFSearch::vcf_search(int depth,char ourColor,move_t * bestMove,move_t * preMove, int preNode, int prePos)
{
    
    if (cloudict_time_up()) return false;
int Color = ourColor;
    int i,j, t;
    int nRoundi0,nRoundi1,nRoundj0,nRoundj1;
    int n_pos=0, n_moveList;
    pos_t canUse[GRID_NUM*GRID_NUM];                            // Points that can be used.
    move_t * moveList = m_vcf_move_list[depth];
    bool flag;
    int NodeID;
    int CurPos=m_vcf_node++;
    char tmpboard[GRID_NUM][GRID_NUM];

    memcpy(tmpboard,m_board,sizeof(tmpboard));

    if (m_has_win)
    {
        for (int i=m_vcf_now_pos+1;i<=m_vcf_total_node;i++)
        {
            if (m_vcf_move_table[i].pre==m_vcf_now_pos&&(m_vcf_move_table[i].p1.x==preMove->positions[0].x&&
                m_vcf_move_table[i].p1.y==preMove->positions[0].y&&m_vcf_move_table[i].p2.x==preMove->positions[1].x&&
                m_vcf_move_table[i].p2.y==preMove->positions[1].y||m_vcf_move_table[i].p2.x==preMove->positions[0].x&&
                m_vcf_move_table[i].p2.y==preMove->positions[0].y&&m_vcf_move_table[i].p1.x==preMove->positions[1].x&&
                m_vcf_move_table[i].p1.y==preMove->positions[1].y))
            {
                bestMove->positions[0]=m_vcf_move_table[m_vcf_move_table[i].next].p1;
                bestMove->positions[1]=m_vcf_move_table[m_vcf_move_table[i].next].p2;
                m_vcf_now_pos=m_vcf_move_table[i].next;
                return true;
            }
        }
        m_has_win=0;
    }

    if (depth==0)
    {
        memcpy(m_org_board,tmpboard,sizeof(tmpboard));
        memset(m_hash_head, -1, sizeof(m_hash_head));
        memset(m_hash_next, -1, sizeof(m_hash_next));
        m_vcf_total_node=0;
    }

    m_hash_que[CurPos].move=*preMove;
    m_hash_que[CurPos].pre=prePos;
    m_hash_que[CurPos].hash=vcf_hash_board(tmpboard);
    m_hash_que[CurPos].dep=depth;
    m_hash_que[CurPos].color=Color;
    if ((t=vcf_hash_check(m_hash_que[CurPos]))!=-1)
    {
        return m_hash_que[t].res;
    }

    if (depth>=VCFDEPTH)                            // Return if it exceed the depth.
    {
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;
    }

    if (depth>=10&&m_vcf_node>500000)
    {
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;
    }

    if (is_win_by_premove(m_board, preMove))        // Won by pre move.
    {
        m_hash_que[CurPos].res=(Color!=(m_chess_type));
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return Color!=(m_chess_type);
    }

    if (Color==(m_chess_type) && is_attack(m_board,Color^3,preMove))
    {

        // Find the points that can be used.
        nRoundi0 = BOARD_SIZE;
        nRoundj0 = BOARD_SIZE;
        nRoundi1 = 1;
        nRoundj1 = 1;

        for (i = 1; i <= BOARD_SIZE; i++)
        {
            for (j = 1; j <= BOARD_SIZE; j++)
            {
                if (m_board[i][j] != NOSTONE)
                {
                    if (i < nRoundi0)
                    {
                        nRoundi0 = i;
                    }
                    if (i > nRoundi1)
                    {
                        nRoundi1 = i;
                    }
                    if (j < nRoundj0)
                    {
                        nRoundj0 = j;
                    }
                    if (j > nRoundj1)
                    {
                        nRoundj1 = j;
                    }
                }
            }
        }
        // Exceed two points for edges.
        nRoundi0 -= 2;
        nRoundj0 -= 2;
        nRoundi1 += 2;
        nRoundj1 += 2;
        if (nRoundi0 < 1)
        {
            nRoundi0 = 1;
        }
        if (nRoundi1 > BOARD_SIZE)
        {
            nRoundi1 = BOARD_SIZE;
        }
        if (nRoundj0 < 1)
        {
            nRoundj0 = 1;
        }
        if (nRoundj1 > BOARD_SIZE)
        {
            nRoundj1 = BOARD_SIZE;
        }


        for (i = nRoundi0; i <= nRoundi1; i++)
        {
            for (j = nRoundj0; j <= nRoundj1; j++)
            {
                if (m_board[i][j] == NOSTONE)
                {
                    canUse[n_pos].x=i;
                    canUse[n_pos].y=j;
                    n_pos++;
                }
            }
        }

        n_moveList=vcf_get_move_list(Color,1,canUse,n_pos,moveList, preMove);

        sort(moveList,n_moveList,preMove);

        for (i=0;i<n_moveList;i++)
        {
            if (moveList[i].score>=50)
            {
                NodeID=++m_vcf_total_node;
                if (depth==0)
                {
                    *bestMove=moveList[i];
                    m_has_win=1;
                    m_vcf_now_pos=NodeID;
                }
                m_vcf_move_table[NodeID].pre=preNode;
                m_vcf_move_table[preNode].next=NodeID;
                m_vcf_move_table[NodeID].p1=moveList[i].positions[0];
                m_vcf_move_table[NodeID].p2=moveList[i].positions[1];

                m_hash_que[CurPos].res=true;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }

                return true;
            }
            make_move(m_board, moveList+i, Color);
            if (is_attack(m_board,Color^3,preMove))
            {
                unmake_move(m_board,moveList+i);
                continue;
            }
            NodeID=++m_vcf_total_node;
            flag = vcf_search(depth+1,ourColor^3,bestMove,moveList+i,NodeID,CurPos); // Recursive search.
            unmake_move(m_board,moveList+i);
            if (flag)        // Win, return.
            {
                if (depth==0)
                {
                    *bestMove=moveList[i];
                    m_has_win=1;
                    m_vcf_now_pos=NodeID;
                }
                m_vcf_move_table[NodeID].pre=preNode;
                m_vcf_move_table[preNode].next=NodeID;
                m_vcf_move_table[NodeID].p1=moveList[i].positions[0];
                m_vcf_move_table[NodeID].p2=moveList[i].positions[1];

                m_hash_que[CurPos].res=true;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }

                return true;
            }
            m_vcf_total_node=NodeID-1;
        }
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;        // Failed.
    }

    if ((Color^3)==(m_chess_type)&& is_attack(m_board,Color^3,preMove)>2)
    {
        m_hash_que[CurPos].res=true;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return true;
    }

    if (Color==(m_chess_type))
    {
        nRoundi0 = BOARD_SIZE;
        nRoundj0 = BOARD_SIZE;
        nRoundi1 = 1;
        nRoundj1 = 1;

        for (i = 1; i <= BOARD_SIZE; i++)
        {
            for (j = 1; j <= BOARD_SIZE; j++)
            {
                if (m_board[i][j] != NOSTONE)
                {
                    if (i < nRoundi0)
                    {
                        nRoundi0 = i;
                    }
                    if (i > nRoundi1)
                    {
                        nRoundi1 = i;
                    }
                    if (j < nRoundj0)
                    {
                        nRoundj0 = j;
                    }
                    if (j > nRoundj1)
                    {
                        nRoundj1 = j;
                    }
                }
            }
        }
        nRoundi0 -= 2;
        nRoundj0 -= 2;
        nRoundi1 += 2;
        nRoundj1 += 2;
        if (nRoundi0 < 1)
        {
            nRoundi0 = 1;
        }
        if (nRoundi1 > BOARD_SIZE)
        {
            nRoundi1 = BOARD_SIZE;
        }
        if (nRoundj0 < 1)
        {
            nRoundj0 = 1;
        }
        if (nRoundj1 > BOARD_SIZE)
        {
            nRoundj1 = BOARD_SIZE;
        }


        for (i = nRoundi0; i <= nRoundi1; i++)
        {
            for (j = nRoundj0; j <= nRoundj1; j++)
            {
                if (m_board[i][j] == NOSTONE)
                {
                    canUse[n_pos].x=i;
                    canUse[n_pos].y=j;
                    n_pos++;
                }
            }
        }

    }

    if ( Color==m_chess_type )
    {
        n_moveList=vcf_get_move_list(Color,1,canUse,n_pos,moveList, preMove);

        sort(moveList,n_moveList,preMove);

        for (i=0;i<n_moveList;i++)
        {
            NodeID=++m_vcf_total_node;
            make_move(m_board, moveList+i, Color);
            flag = vcf_search(depth+1,ourColor^3,bestMove,moveList+i,NodeID,CurPos);
            unmake_move(m_board,moveList+i);
            if (flag)
            {
                if (depth==0)
                {

                    *bestMove=moveList[i];
                    m_has_win=1;
                    m_vcf_now_pos=NodeID;
                }
                m_vcf_move_table[NodeID].pre=preNode;
                m_vcf_move_table[preNode].next=NodeID;
                m_vcf_move_table[NodeID].p1=moveList[i].positions[0];
                m_vcf_move_table[NodeID].p2=moveList[i].positions[1];
                m_hash_que[CurPos].res=true;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
                return true;
            }
            m_vcf_total_node=NodeID-1;
        }
        m_hash_que[CurPos].res=false;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return false;
    }
    else
    {

        n_moveList=vcf_get_move_list(Color,0,canUse,n_pos,moveList, preMove);
        j=m_vcf_total_node;
        for (i=0;i<n_moveList;i++)
        {
            NodeID=++m_vcf_total_node;
            make_move(m_board, moveList+i, Color);
            flag = vcf_search(depth+1,ourColor^3,bestMove,moveList+i,NodeID,CurPos);
            unmake_move(m_board,moveList+i);
            if ( !flag )
            {
                m_vcf_total_node=j;
                m_hash_que[CurPos].res=false;
                { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
                return false;
            }
            m_vcf_move_table[NodeID].pre=preNode;
            m_vcf_move_table[NodeID].p1=moveList[i].positions[0];
            m_vcf_move_table[NodeID].p2=moveList[i].positions[1];
        }
        m_hash_que[CurPos].res=true;
        { int __b = (int)(m_hash_que[CurPos].hash % HASHSIZE); m_hash_next[CurPos] = m_hash_head[depth][__b]; m_hash_head[depth][__b] = CurPos; }
        return true;
    }
}


void print_eval() {}


// ======================= Botzone 15x15 JSON adapter =======================
struct BZMove {
    int x0 = -1, y0 = -1, x1 = -1, y1 = -1;
};

static int extract_int_after_key(const std::string &obj, const char *key) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = obj.find(k);
    if (p == std::string::npos) return -1;
    p = obj.find(':', p);
    if (p == std::string::npos) return -1;
    ++p;
    while (p < obj.size() && std::isspace((unsigned char)obj[p])) ++p;
    int sign = 1;
    if (p < obj.size() && obj[p] == '-') { sign = -1; ++p; }
    int v = 0;
    while (p < obj.size() && std::isdigit((unsigned char)obj[p])) {
        v = v * 10 + (obj[p] - '0');
        ++p;
    }
    return sign * v;
}

static std::vector<BZMove> parse_section(const std::string &s, const std::string &name) {
    std::vector<BZMove> out;
    std::string key = "\"" + name + "\"";
    size_t p = s.find(key);
    if (p == std::string::npos) return out;
    p = s.find('[', p);
    if (p == std::string::npos) return out;
    int depth = 0;
    size_t start = p, end = std::string::npos;
    for (size_t i = p; i < s.size(); ++i) {
        if (s[i] == '[') depth++;
        else if (s[i] == ']') {
            depth--;
            if (depth == 0) { end = i; break; }
        }
    }
    if (end == std::string::npos) return out;
    size_t i = start + 1;
    while (i < end) {
        if (s[i] == '{') {
            int d = 1;
            size_t j = i + 1;
            while (j < end && d > 0) {
                if (s[j] == '{') d++;
                else if (s[j] == '}') d--;
                ++j;
            }
            std::string obj = s.substr(i, j - i);
            BZMove m;
            m.x0 = extract_int_after_key(obj, "x0");
            m.y0 = extract_int_after_key(obj, "y0");
            m.x1 = extract_int_after_key(obj, "x1");
            m.y1 = extract_int_after_key(obj, "y1");
            out.push_back(m);
            i = j;
        } else {
            ++i;
        }
    }
    return out;
}

static inline bool bz_in_board(int x, int y) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

static bool apply_bz_move(char board[][GRID_NUM], const BZMove &m, char color) {
    if (m.x0 < 0 || m.y0 < 0) return true;
    if (!bz_in_board(m.x0, m.y0)) return false;
    int ax = m.x0 + 1, ay = m.y0 + 1;
    if (board[ax][ay] != NOSTONE) return false;
    board[ax][ay] = color;
    if (m.x1 >= 0 && m.y1 >= 0) {
        if (!bz_in_board(m.x1, m.y1)) return false;
        int bx = m.x1 + 1, by = m.y1 + 1;
        if (bx == ax && by == ay) return false;
        if (board[bx][by] != NOSTONE) return false;
        board[bx][by] = color;
    }
    return true;
}

static move_t bz_to_cloudict_move(const BZMove &m) {
    move_t mv;
    mv.positions[0].x = m.x0 + 1;
    mv.positions[0].y = m.y0 + 1;
    if (m.x1 >= 0 && m.y1 >= 0) {
        mv.positions[1].x = m.x1 + 1;
        mv.positions[1].y = m.y1 + 1;
    } else {
        mv.positions[1] = mv.positions[0];
    }
    mv.score = 0;
    return mv;
}

static bool legal_cloudict_pair(char board[][GRID_NUM], const move_t &m, bool single) {
    int ax = m.positions[0].x, ay = m.positions[0].y;
    int bx = m.positions[1].x, by = m.positions[1].y;
    if (!IsValidPos(ax, ay) || board[ax][ay] != NOSTONE) return false;
    if (single) return true;
    if (!IsValidPos(bx, by) || board[bx][by] != NOSTONE) return false;
    if (ax == bx && ay == by) return false;
    return true;
}

static int count_stones(char board[][GRID_NUM]) {
    int c = 0;
    for (int i = 1; i < GRID_NUM - 1; ++i)
        for (int j = 1; j < GRID_NUM - 1; ++j)
            if (board[i][j] != NOSTONE) ++c;
    return c;
}

static move_t fallback_move(char board[][GRID_NUM], bool single) {
    move_t m; memset(&m, 0, sizeof(m));
    static const int pref[][2] = {
        {8,8},{8,7},{7,8},{9,8},{8,9},{7,7},{9,9},{7,9},{9,7},
        {6,8},{8,6},{10,8},{8,10},{6,6},{10,10},{6,10},{10,6}
    };
    std::vector<pos_t> empties;
    for (auto &p : pref) {
        int x = p[0], y = p[1];
        if (IsValidPos(x,y) && board[x][y] == NOSTONE) empties.push_back({x,y});
    }
    for (int i=1;i<GRID_NUM-1;i++)
        for (int j=1;j<GRID_NUM-1;j++)
            if (board[i][j] == NOSTONE) empties.push_back({i,j});
    if (empties.empty()) {
        m.positions[0] = {1,1};
        m.positions[1] = {1,1};
        return m;
    }
    m.positions[0] = empties[0];
    if (single) m.positions[1] = empties[0];
    else m.positions[1] = empties.size() > 1 ? empties[1] : empties[0];
    return m;
}


// ======================= Elite tactical layer =======================
// These routines do NOT replace Cloudict. They sit before VCF/alpha-beta and
// catch exact one-move wins and one-move opponent wins by scanning every 6-cell
// line segment on the 15x15 board. This avoids losing strength when Cloudict's
// narrow candidate generator misses a forced tactical square under 1s.
static inline bool same_pos(pos_t a, pos_t b) { return a.x == b.x && a.y == b.y; }

static int center_bonus_pos(pos_t p) {
    int cx = BOARD_SIZE/2 + 1, cy = BOARD_SIZE/2 + 1;
    int dx = p.x - cx; if (dx < 0) dx = -dx;
    int dy = p.y - cy; if (dy < 0) dy = -dy;
    return 32 - dx - dy;
}

static pos_t aux_empty_after(char board[][GRID_NUM], pos_t forbid) {
    static const int pref[][2] = {
        {8,8},{8,7},{7,8},{9,8},{8,9},{7,7},{9,9},{7,9},{9,7},
        {6,8},{8,6},{10,8},{8,10},{6,6},{10,10},{6,10},{10,6},
        {5,8},{8,5},{11,8},{8,11},{5,5},{11,11},{5,11},{11,5}
    };
    for (auto &q : pref) {
        pos_t p{q[0], q[1]};
        if (IsValidPos(p.x,p.y) && !same_pos(p, forbid) && board[p.x][p.y] == NOSTONE) return p;
    }
    for (int i=1;i<GRID_NUM-1;i++) for (int j=1;j<GRID_NUM-1;j++) {
        pos_t p{i,j};
        if (!same_pos(p, forbid) && board[i][j] == NOSTONE) return p;
    }
    return forbid;
}

static bool add_unique_pos(std::vector<pos_t> &v, pos_t p) {
    for (auto q : v) if (same_pos(q,p)) return false;
    v.push_back(p);
    return true;
}

static int tactical_pair_score(pos_t a, pos_t b) {
    return center_bonus_pos(a) + center_bonus_pos(b) - (same_pos(a,b) ? 1000 : 0);
}

static bool exact_window_win_pair(char board[][GRID_NUM], char color, move_t *best) {
    static const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    bool found = false;
    int bestScore = -100000000;
    move_t cand; memset(&cand,0,sizeof(cand));
    for (int x=1; x<=BOARD_SIZE; ++x) for (int y=1; y<=BOARD_SIZE; ++y) {
        for (auto &d : dirs) {
            int dx=d[0], dy=d[1];
            int ex=x+5*dx, ey=y+5*dy;
            if (!IsValidPos(ex,ey)) continue;
            int own=0, bad=0;
            pos_t emp[6]; int ec=0;
            for (int k=0;k<6;k++) {
                int xx=x+k*dx, yy=y+k*dy;
                if (board[xx][yy] == color) own++;
                else if (board[xx][yy] == NOSTONE) emp[ec++] = {xx,yy};
                else bad++;
            }
            if (bad != 0) continue;
            if (own + ec != 6) continue;
            if (ec == 1 || ec == 2) {
                move_t m; memset(&m,0,sizeof(m));
                m.positions[0] = emp[0];
                m.positions[1] = (ec == 2 ? emp[1] : aux_empty_after(board, emp[0]));
                if (!legal_cloudict_pair(board, m, false)) continue;
                int sc = 1000000 + own*1000 + tactical_pair_score(m.positions[0], m.positions[1]);
                if (!found || sc > bestScore) { found=true; bestScore=sc; cand=m; }
            }
        }
    }
    if (found) { *best = cand; return true; }
    return false;
}

struct ThreatWinWindow { pos_t a; pos_t b; bool single; };

static void collect_win_windows(char board[][GRID_NUM], char color, std::vector<ThreatWinWindow> &threats, std::vector<pos_t> &cands) {
    static const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    for (int x=1; x<=BOARD_SIZE; ++x) for (int y=1; y<=BOARD_SIZE; ++y) {
        for (auto &d : dirs) {
            int dx=d[0], dy=d[1];
            int ex=x+5*dx, ey=y+5*dy;
            if (!IsValidPos(ex,ey)) continue;
            int own=0, bad=0;
            pos_t emp[6]; int ec=0;
            for (int k=0;k<6;k++) {
                int xx=x+k*dx, yy=y+k*dy;
                if (board[xx][yy] == color) own++;
                else if (board[xx][yy] == NOSTONE) emp[ec++] = {xx,yy};
                else bad++;
            }
            if (bad == 0 && own + ec == 6 && (ec == 1 || ec == 2)) {
                ThreatWinWindow tw;
                tw.a = emp[0];
                tw.b = (ec == 2 ? emp[1] : emp[0]);
                tw.single = (ec == 1);
                threats.push_back(tw);
                add_unique_pos(cands, tw.a);
                if (!tw.single) add_unique_pos(cands, tw.b);
            }
        }
    }
}

static bool covers_threat(pos_t a, pos_t b, const ThreatWinWindow &t) {
    return same_pos(a,t.a) || same_pos(b,t.a) || (!t.single && (same_pos(a,t.b) || same_pos(b,t.b)));
}

static bool exact_block_opponent_win(char board[][GRID_NUM], char oppColor, move_t *best) {
    std::vector<ThreatWinWindow> threats;
    std::vector<pos_t> cands;
    collect_win_windows(board, oppColor, threats, cands);
    if (threats.empty()) return false;
    if (cands.empty()) return false;

    int bestCover = -1;
    int bestScore = -100000000;
    move_t ans; memset(&ans,0,sizeof(ans));

    // Try two blocking stones from all urgent squares. Usually this is tiny; even in wild
    // positions it is bounded by the empty endpoints of immediate six-windows.
    for (size_t i=0;i<cands.size();++i) {
        for (size_t j=i+1;j<cands.size();++j) {
            pos_t a=cands[i], b=cands[j];
            if (board[a.x][a.y] != NOSTONE || board[b.x][b.y] != NOSTONE || same_pos(a,b)) continue;
            int cover=0;
            for (auto &t : threats) if (covers_threat(a,b,t)) cover++;
            int sc = cover*100000 + tactical_pair_score(a,b);
            if (cover > bestCover || (cover == bestCover && sc > bestScore)) {
                bestCover = cover; bestScore = sc;
                ans.positions[0]=a; ans.positions[1]=b; ans.score=sc;
            }
        }
    }

    // If there is only one urgent point, pair it with the best legal auxiliary point.
    if (bestCover < 0 && cands.size() == 1) {
        pos_t a=cands[0], b=aux_empty_after(board,a);
        if (!same_pos(a,b) && board[a.x][a.y] == NOSTONE && board[b.x][b.y] == NOSTONE) {
            ans.positions[0]=a; ans.positions[1]=b; ans.score=100000;
            bestCover = (int)threats.size();
        }
    }

    if (bestCover >= 0 && legal_cloudict_pair(board, ans, false)) { *best = ans; return true; }
    return false;
}


// Count how many immediate winning windows would remain after the opponent uses
// the best possible two stones to defend. If this is > 0, the side to move has
// created an unavoidable next-turn win under Connect6's two-stone move rule.
static int max_cover_by_two_stones(const std::vector<ThreatWinWindow> &threats, const std::vector<pos_t> &cands) {
    if (threats.empty()) return 0;
    if (cands.empty()) return 0;
    int best = 0;
    for (size_t i=0;i<cands.size();++i) {
        for (size_t j=i;j<cands.size();++j) {
            int cover = 0;
            pos_t a = cands[i], b = cands[j];
            for (auto &t : threats) {
                if (covers_threat(a,b,t)) cover++;
            }
            if (cover > best) best = cover;
        }
    }
    return best;
}

static int residual_forced_win_windows(char board[][GRID_NUM], char color, int *threatCountOut = NULL, int *candCountOut = NULL) {
    std::vector<ThreatWinWindow> threats;
    std::vector<pos_t> cands;
    collect_win_windows(board, color, threats, cands);
    if (threatCountOut) *threatCountOut = (int)threats.size();
    if (candCountOut) *candCountOut = (int)cands.size();
    if (threats.empty()) return 0;
    int mx = max_cover_by_two_stones(threats, cands);
    return (int)threats.size() - mx;
}

static int point_window_potential(char board[][GRID_NUM], char color, char oppColor, pos_t p) {
    static const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    int score = center_bonus_pos(p);
    for (auto &d : dirs) {
        int dx=d[0], dy=d[1];
        for (int shift=0; shift<6; ++shift) {
            int sx = p.x - shift*dx;
            int sy = p.y - shift*dy;
            int ex = sx + 5*dx;
            int ey = sy + 5*dy;
            if (!IsValidPos(sx,sy) || !IsValidPos(ex,ey)) continue;
            int own=1, opp=0, emp=0; // assume p is placed by color
            for (int k=0;k<6;k++) {
                int xx=sx+k*dx, yy=sy+k*dy;
                if (xx==p.x && yy==p.y) continue;
                if (board[xx][yy] == color) own++;
                else if (board[xx][yy] == oppColor) opp++;
                else emp++;
            }
            if (opp == 0) {
                if (own >= 5) score += 200000;
                else if (own == 4) score += 8000;
                else if (own == 3) score += 800;
                else if (own == 2) score += 80;
            }
            // Defensive value: placing at p ruins an opponent segment.
            int opcnt=0, myblock=0;
            for (int k=0;k<6;k++) {
                int xx=sx+k*dx, yy=sy+k*dy;
                if (xx==p.x && yy==p.y) continue;
                if (board[xx][yy] == oppColor) opcnt++;
                else if (board[xx][yy] == color) myblock++;
            }
            if (myblock == 0) {
                if (opcnt >= 5) score += 300000;
                else if (opcnt == 4) score += 12000;
                else if (opcnt == 3) score += 1000;
                else if (opcnt == 2) score += 100;
            }
        }
    }
    return score;
}

static void elite_candidate_points(char board[][GRID_NUM], char myColor, char oppColor, std::vector<pos_t> &out) {
    bool hasStone = false;
    bool mark[GRID_NUM][GRID_NUM]; memset(mark,0,sizeof(mark));
    for (int x=1;x<=BOARD_SIZE;x++) for (int y=1;y<=BOARD_SIZE;y++) {
        if (board[x][y] == NOSTONE) continue;
        hasStone = true;
        for (int dx=-2; dx<=2; ++dx) for (int dy=-2; dy<=2; ++dy) {
            int nx=x+dx, ny=y+dy;
            if (IsValidPos(nx,ny) && board[nx][ny] == NOSTONE && !mark[nx][ny]) {
                mark[nx][ny] = true;
                out.push_back({nx,ny});
            }
        }
    }
    if (!hasStone) {
        out.push_back({BOARD_SIZE/2+1, BOARD_SIZE/2+1});
        return;
    }
    // Always keep a small central shell, because opening fights on 15x15 are
    // very sensitive to central influence.
    for (int x=BOARD_SIZE/2-2+1; x<=BOARD_SIZE/2+2+1; ++x) {
        for (int y=BOARD_SIZE/2-2+1; y<=BOARD_SIZE/2+2+1; ++y) {
            if (IsValidPos(x,y) && board[x][y] == NOSTONE && !mark[x][y]) {
                mark[x][y] = true;
                out.push_back({x,y});
            }
        }
    }
    std::sort(out.begin(), out.end(), [&](const pos_t &a, const pos_t &b) {
        int sa = point_window_potential(board,myColor,oppColor,a);
        int sb = point_window_potential(board,myColor,oppColor,b);
        return sa > sb;
    });
    const size_t LIMIT = 34; // 561 pairs; cheap on 15x15 and safely below 1s.
    if (out.size() > LIMIT) out.resize(LIMIT);
}

static bool elite_unavoidable_threat_pair(char board[][GRID_NUM], char myColor, char oppColor, move_t *best) {
    std::vector<pos_t> cands;
    elite_candidate_points(board, myColor, oppColor, cands);
    if (cands.size() < 2) return false;

    bool found = false;
    long long bestScore = -9000000000000000000LL;
    move_t ans; memset(&ans,0,sizeof(ans));

    for (size_t i=0;i<cands.size();++i) {
        for (size_t j=i+1;j<cands.size();++j) {
            move_t m; memset(&m,0,sizeof(m));
            m.positions[0] = cands[i];
            m.positions[1] = cands[j];
            if (!legal_cloudict_pair(board, m, false)) continue;

            make_move(board, &m, myColor);

            // Do not choose an attacking threat move if the opponent can simply
            // win immediately on the next turn.
            move_t tmp; memset(&tmp,0,sizeof(tmp));
            bool oppCanWinNow = exact_window_win_pair(board, oppColor, &tmp);

            int th=0, dc=0;
            int residual = oppCanWinNow ? 0 : residual_forced_win_windows(board, myColor, &th, &dc);

            // A lower-priority positional bonus only breaks ties after the exact
            // forced-threat proof.
            long long sc = (long long)residual * 1000000000LL
                         + (long long)th * 1000000LL
                         + (long long)dc * 10000LL
                         + point_window_potential(board, myColor, oppColor, cands[i])
                         + point_window_potential(board, myColor, oppColor, cands[j]);

            unmake_move(board, &m);

            if (residual > 0 && (!found || sc > bestScore)) {
                found = true;
                bestScore = sc;
                ans = m;
                ans.score = (int)std::min<long long>(MAXINT-2, sc/1000000LL);
            }
        }
    }

    if (found && legal_cloudict_pair(board, ans, false)) {
        *best = ans;
        return true;
    }
    return false;
}



static bool elite_has_unavoidable_threat_pair_limited(char board[][GRID_NUM], char atkColor, char defColor, int limitN) {
    std::vector<pos_t> cands;
    elite_candidate_points(board, atkColor, defColor, cands);
    if ((int)cands.size() > limitN) cands.resize(limitN);
    if (cands.size() < 2) return false;
    for (size_t i=0;i<cands.size();++i) {
        for (size_t j=i+1;j<cands.size();++j) {
            move_t m; memset(&m,0,sizeof(m));
            m.positions[0]=cands[i];
            m.positions[1]=cands[j];
            if (!legal_cloudict_pair(board,m,false)) continue;
            make_move(board,&m,atkColor);
            bool winNow = is_win_by_premove(board,&m);
            int residual = winNow ? 1 : residual_forced_win_windows(board, atkColor, NULL, NULL);
            unmake_move(board,&m);
            if (residual > 0) return true;
        }
    }
    return false;
}

static bool elite_prevent_opponent_forced_pair(char board[][GRID_NUM], char myColor, char oppColor, move_t *best) {
    // Only activate this expensive defensive layer if the opponent currently
    // has a two-stone move that creates an unavoidable next-turn win. This is
    // the key anti-TSS guard that prevents losing one ply before a direct block.
    if (!elite_has_unavoidable_threat_pair_limited(board, oppColor, myColor, 18)) return false;

    std::vector<pos_t> cands;
    elite_candidate_points(board, myColor, oppColor, cands);
    if ((int)cands.size() > 24) cands.resize(24);

    bool foundSafe = false;
    long long bestScore = -9000000000000000000LL;
    move_t ans; memset(&ans,0,sizeof(ans));

    for (size_t i=0;i<cands.size();++i) {
        for (size_t j=i+1;j<cands.size();++j) {
            move_t m; memset(&m,0,sizeof(m));
            m.positions[0]=cands[i];
            m.positions[1]=cands[j];
            if (!legal_cloudict_pair(board,m,false)) continue;

            make_move(board,&m,myColor);

            move_t tmp; memset(&tmp,0,sizeof(tmp));
            bool oppDirect = exact_window_win_pair(board, oppColor, &tmp);
            bool oppForced = false;
            if (!oppDirect) oppForced = elite_has_unavoidable_threat_pair_limited(board, oppColor, myColor, 16);

            int myTh=0,myCand=0;
            int myResidual = residual_forced_win_windows(board,myColor,&myTh,&myCand);
            long long sc = (long long)myResidual * 1000000000LL
                         + (long long)myTh * 1000000LL
                         + (long long)myCand * 10000LL
                         + point_window_potential(board,myColor,oppColor,cands[i])
                         + point_window_potential(board,myColor,oppColor,cands[j]);

            unmake_move(board,&m);

            if (!oppDirect && !oppForced) {
                if (!foundSafe || sc > bestScore) {
                    foundSafe = true;
                    bestScore = sc;
                    ans = m;
                    ans.score = (int)std::min<long long>(MAXINT-3, sc/1000000LL);
                }
            }
        }
    }

    if (foundSafe && legal_cloudict_pair(board, ans, false)) {
        *best = ans;
        return true;
    }
    return false;
}


static bool elite_immediate_tactics(char board[][GRID_NUM], char myColor, char oppColor, move_t *best) {
    // Never miss a direct winning move.
    if (exact_window_win_pair(board, myColor, best)) return true;
    // If no win exists, block opponent's direct win windows.
    if (exact_block_opponent_win(board, oppColor, best)) return true;
    return false;
}

static bool immediate_win_by_generator(char board[][GRID_NUM], char color, move_t *best) {
    CMoveGenerator gen;
    move_t list[NUMOFONE * NUMOFTWO * 2];
    int n = gen.get_move_list(color, list, board);
    for (int i = 0; i < n; ++i) {
        if (!legal_cloudict_pair(board, list[i], false)) continue;
        make_move(board, &list[i], color);
        bool win = is_win_by_premove(board, &list[i]);
        unmake_move(board, &list[i]);
        if (win) {
            *best = list[i];
            return true;
        }
    }
    return false;
}

static bool sanitize_or_fallback(char board[][GRID_NUM], move_t *best, bool single) {
    if (legal_cloudict_pair(board, *best, single)) return true;
    *best = fallback_move(board, single);
    return legal_cloudict_pair(board, *best, single);
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string input, line;
    while (std::getline(std::cin, line)) input += line;
    if (input.empty()) input = "{\"requests\":[{\"x0\":-1,\"y0\":-1,\"x1\":-1,\"y1\":-1}],\"responses\":[]}";

    std::vector<BZMove> requests = parse_section(input, "requests");
    std::vector<BZMove> responses = parse_section(input, "responses");
    int turnID = (int)responses.size();
    bool selfFirstBlack = (!requests.empty() && requests[0].x0 < 0);
    char myColor = selfFirstBlack ? BLACK : WHITE;
    char oppColor = myColor ^ 3;

    char board[GRID_NUM][GRID_NUM];
    init_board(board);

    move_t preMove; memset(&preMove, 0, sizeof(preMove));
    preMove.positions[0] = {BOARD_SIZE/2 + 1, BOARD_SIZE/2 + 1};
    preMove.positions[1] = preMove.positions[0];

    for (int i = 0; i < turnID && i < (int)requests.size(); ++i) {
        if (requests[i].x0 >= 0) {
            apply_bz_move(board, requests[i], oppColor);
            preMove = bz_to_cloudict_move(requests[i]);
        }
        if (i < (int)responses.size() && responses[i].x0 >= 0) {
            apply_bz_move(board, responses[i], myColor);
            preMove = bz_to_cloudict_move(responses[i]);
        }
    }
    if (turnID < (int)requests.size() && requests[turnID].x0 >= 0) {
        apply_bz_move(board, requests[turnID], oppColor);
        preMove = bz_to_cloudict_move(requests[turnID]);
    }

    bool blackOpening = (turnID == 0 && selfFirstBlack);
    move_t best; memset(&best, 0, sizeof(best));

    if (blackOpening) {
        best.positions[0] = {BOARD_SIZE/2 + 1, BOARD_SIZE/2 + 1};
        best.positions[1] = best.positions[0];
    } else {
        bool found = false;

        // 1) Exact tactical layer: all-board one-move win, then urgent block.
        found = elite_immediate_tactics(board, myColor, oppColor, &best);

        // 1b) Cloudict generator tactical must-win fallback.
        if (!found) found = immediate_win_by_generator(board, myColor, &best);

        // 1c) Exact Connect6 double/triple-threat layer:
        // choose a pair that creates more immediate winning windows than the
        // opponent can parry with two stones. This is the practical 1s version
        // of threat-based / relevance-zone ideas from top Connect6 programs.
        if (!found) found = elite_unavoidable_threat_pair(board, myColor, oppColor, &best);

        // 1d) Anti-threat-space guard: if the opponent is one move away from
        // producing an unavoidable double/triple threat, choose a safe pair now.
        if (!found) found = elite_prevent_opponent_forced_pair(board, myColor, oppColor, &best);

        // 2) Cloudict VCF search, embedded patterns.in, no external files.
        if (!found) {
            CVCFSearch *vcf = new CVCFSearch();
            if (vcf->init()) {
                vcf->init_game();
                vcf->before_search(board, myColor);
                cloudict_set_deadline_seconds(BOTZONE_VCF_SECONDS);
                found = vcf->vcf_search(0, myColor, &best, &preMove, 0, -1);
                cloudict_set_deadline_seconds(0);
                if (found && !legal_cloudict_pair(board, best, false)) found = false;
            }
            delete vcf;
        }

        // 3) Original Cloudict alpha-beta + move generator + evaluation.
        if (!found) {
            CSearchEngine *se = new CSearchEngine();
            int depth = BOTZONE_AB_DEPTH; // strict 1s Botzone-safe default; raise only after local timing tests.
            se->before_search(board, myColor, depth);
            cloudict_set_deadline_seconds(BOTZONE_AB_SECONDS);
            se->alpha_beta_search(depth, MININT, MAXINT, myColor, &best, &preMove);
            cloudict_set_deadline_seconds(0);
            found = legal_cloudict_pair(board, best, false);
            delete se;
        }

        if (!found) best = fallback_move(board, false);
    }

    sanitize_or_fallback(board, &best, blackOpening);

    int x0 = best.positions[0].x - 1;
    int y0 = best.positions[0].y - 1;
    int x1 = blackOpening ? -1 : best.positions[1].x - 1;
    int y1 = blackOpening ? -1 : best.positions[1].y - 1;

    std::cout << "{\"response\":{\"x0\":" << x0
              << ",\"y0\":" << y0
              << ",\"x1\":" << x1
              << ",\"y1\":" << y1
              << "}}" << std::endl;
    return 0;
}