﻿#pragma once
#include "BattleScene.h"
#include "BattleMap.h"
#include "Head.h"
#include "Save.h"
#include <deque>



//j轻攻击，i重攻击，m闪身
//每场战斗可以选择从4种武学中选择轻重
//硬直，判定范围，威力，消耗体力，在不攻击的时候可以回复体力

struct AttackEffect
{
    double X1, Y1;
    Role* Attacker = nullptr;
    std::map<Role*, int> Defender;    //每人只能被一个特效击中一次
    int Frame;
    int EffectNumber;
    int Heavy;
    std::string Path;
};

class BattleSceneHades : public BattleScene
{
public:
    BattleSceneHades();
    BattleSceneHades(int id);
    virtual ~BattleSceneHades();
    void setID(int id);

    //继承自基类的函数
    virtual void draw() override;
    virtual void dealEvent(BP_Event& e) override;     //战场主循环
    virtual void dealEvent2(BP_Event& e) override;    //用于停止自动
    virtual void onEntrance() override;
    virtual void onExit() override;
    virtual void backRun() override;

protected:
    double man_x1_ = 64 * TILE_W, man_y1_ = 64 * TILE_H;

    std::deque<AttackEffect> attack_effects_;

    Point pos45To90(int x, int y)    //45度坐标转为直角
    {
        Point p;
        p.x = -y * TILE_W + x * TILE_W + COORD_COUNT * TILE_W;
        p.y = y * TILE_H + x * TILE_H;
        return p;
    }

    Point pos90To45(double x, double y)    //直角坐标转为45度
    {
        x -= COORD_COUNT * TILE_W;
        Point p;
        p.x = round(((x) / TILE_W + (y) / TILE_H) / 2);
        p.y = round(((-x) / TILE_W + (y) / TILE_H) / 2);
        return p;
    }

    bool canWalk45(int x, int y)
    {
        if (isOutLine(x, y) || isBuilding(x, y) || isWater(x, y))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    bool canWalk90(int x, int y)
    {
        auto p = pos90To45(x, y);
        return canWalk45(p.x, p.y);
    }

    double EuclidDis(double x, double y)
    {
        return sqrt(x * x + 4 * y * y);
    }
    void norm(double& x, double& y)
    {
        auto n = sqrt(x * x + y * y);
        if (n > 0)
        {
            x /= n;
            y /= n;
        }
    }
    bool is_running_ = false;   //主角是否在跑动
    Role* role_;    //主角
    int weapon_ = 1;

};
