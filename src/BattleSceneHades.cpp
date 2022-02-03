﻿#include "BattleSceneHades.h"
#include "Audio.h"
#include "GameUtil.h"
#include "Head.h"
#include "MainScene.h"

BattleSceneHades::BattleSceneHades()
{
    full_window_ = 1;
    COORD_COUNT = BATTLEMAP_COORD_COUNT;

    earth_layer_.resize(COORD_COUNT);
    building_layer_.resize(COORD_COUNT);

    head_self_ = std::make_shared<Head>();
    addChild(head_self_);
    //head_self_->setRole();
}

BattleSceneHades::BattleSceneHades(int id) : BattleSceneHades()
{
    setID(id);
}

BattleSceneHades::~BattleSceneHades()
{
}

void BattleSceneHades::setID(int id)
{
    battle_id_ = id;
    info_ = BattleMap::getInstance()->getBattleInfo(id);

    BattleMap::getInstance()->copyLayerData(info_->BattleFieldID, 0, &earth_layer_);
    BattleMap::getInstance()->copyLayerData(info_->BattleFieldID, 1, &building_layer_);
}

void BattleSceneHades::draw()
{
    //在这个模式下，使用的是直角坐标
    Engine::getInstance()->setRenderAssistTexture();
    Engine::getInstance()->fillColor({ 0, 0, 0, 255 }, 0, 0, render_center_x_ * 2, render_center_y_ * 2);

    //以下是计算出需要画的区域，先画到一个大图上，再转贴到窗口
    {
        auto p = pos90To45(man_x1_, man_y1_);
        man_x_ = p.x;
        man_y_ = p.y;
    }
    //一整块地面
    if (earth_texture_)
    {
        Engine::getInstance()->setRenderTarget(earth_texture_);
        for (int sum = -view_sum_region_; sum <= view_sum_region_ + 15; sum++)
        {
            for (int i = -view_width_region_; i <= view_width_region_; i++)
            {
                int ix = man_x_ + i + (sum / 2);
                int iy = man_y_ - i + (sum - sum / 2);
                auto p = pos45To90(ix, iy);
                if (!isOutLine(ix, iy))
                {
                    int num = earth_layer_.data(ix, iy) / 2;
                    BP_Color color = { 255, 255, 255, 255 };
                    bool need_draw = true;
                    if (need_draw && num > 0)
                    {
                        TextureManager::getInstance()->renderTexture("smap", num, p.x, p.y, color);
                    }
                }
            }
        }

        struct DrawInfo
        {
            std::string path;
            int num;
            Point p;
        };
        std::vector<DrawInfo> building_vec;
        building_vec.reserve(10000);

        for (int sum = -view_sum_region_; sum <= view_sum_region_ + 15; sum++)
        {
            for (int i = -view_width_region_; i <= view_width_region_; i++)
            {

                int ix = man_x_ + i + (sum / 2);
                int iy = man_y_ - i + (sum - sum / 2);
                auto p = pos45To90(ix, iy);
                if (!isOutLine(ix, iy))
                {
                    int num = building_layer_.data(ix, iy) / 2;
                    if (num > 0)
                    {
                        TextureManager::getInstance()->renderTexture("smap", num, p.x, p.y);
                        building_vec.emplace_back("smap", num, p);
                    }
                }
            }
        }

        for (auto r : battle_roles_)
        {
            std::string path = fmt1::format("fight/fight{:03}", r->HeadID);
            BP_Color color = { 255, 255, 255, 255 };
            uint8_t alpha = 255;
            if (battle_cursor_->isRunning() && !acting_role_->isAuto())
            {
                color = { 128, 128, 128, 255 };
                if (inEffect(acting_role_, r))
                {
                    color = { 255, 255, 255, 255 };
                }
            }
            int pic;
            if (r == acting_role_)
            {
                pic = calRolePic(r, action_type_, action_frame_);
            }
            else
            {
                pic = calRolePic(r);
            }
            if (r->HP <= 0)
            {
                alpha = dead_alpha_;
            }
            pic = calRolePic(r, r->ActType, r->ActFrame);
            //TextureManager::getInstance()->renderTexture(path, pic, r->X1, r->Y1, color, alpha);
            building_vec.emplace_back(path, pic, Point{ int(round(r->X1)), int(round(r->Y1)) });
            //renderExtraRoleInfo(r, r->X1, r->Y1);
        }

        auto sort_building = [](DrawInfo& d1, DrawInfo& d2)
        {
            if (d1.p.y != d2.p.y)
                return d1.p.y < d2.p.y;
            else
                return d1.p.x < d2.p.x;
        };
        std::sort(building_vec.begin(), building_vec.end(), sort_building);
        for (auto& d : building_vec)
        {
            TextureManager::getInstance()->renderTexture(d.path, d.num, d.p.x, d.p.y);
        }

        BP_Color c = { 255, 255, 255, 255 };
        Engine::getInstance()->setColor(earth_texture_, c);
        int w = render_center_x_ * 2;
        int h = render_center_y_ * 2;
        //获取的是中心位置，如贴图应减掉屏幕尺寸的一半
        BP_Rect rect0 = { int(man_x1_ - render_center_x_ - x_), int(man_y1_ - render_center_y_ - y_), w, h }, rect1 = { 0, 0, w, h };

        //effects
        for (auto& ae : attack_effects_)
        {
            TextureManager::getInstance()->renderTexture(ae.Path, ae.Frame, ae.X1, ae.Y1, { 255, 255, 255, 255 }, 224);
        }


        Engine::getInstance()->setRenderAssistTexture();
        Engine::getInstance()->renderCopy(earth_texture_, &rect0, &rect1, 1);


    }

    Engine::getInstance()->renderAssistTextureToWindow();

    if (result_ >= 0)
    {
        Engine::getInstance()->fillColor({ 0, 0, 0, 128 }, 0, 0, -1, -1);
    }
}

void BattleSceneHades::dealEvent(BP_Event& e)
{
    auto engine = Engine::getInstance();
    auto r = role_;


    //方向
    //需要注意计算欧氏距离时y方向需乘2计入，但主角的操作此时方向是离散的

    man_x1_ = r->X1;
    man_y1_ = r->Y1;
    if (r->CoolDown == 0)
    {
        if (current_frame_ % 3 == 0)
        {
            if (engine->checkKeyPress(BPK_a))
            {
                man_x1_ -= 2;
                r->FaceTowards = Towards_LeftDown;
                r->ActType2 = 0;
            }
            if (engine->checkKeyPress(BPK_d))
            {
                man_x1_ += 2;
                r->FaceTowards = Towards_RightUp;
                r->ActType2 = 0;
            }
            if (engine->checkKeyPress(BPK_w))
            {
                man_y1_ -= 1;
                r->FaceTowards = Towards_LeftUp;
                r->ActType2 = 0;
            }
            if (engine->checkKeyPress(BPK_s))
            {
                man_y1_ += 1;
                r->FaceTowards = Towards_RightDown;
                r->ActType2 = 0;
            }
        }
        if (engine->checkKeyPress(BPK_1))
        {
            weapon_ = 1;
        }
        if (engine->checkKeyPress(BPK_2))
        {
            weapon_ = 2;
        }
        if (engine->checkKeyPress(BPK_3))
        {
            weapon_ = 3;
        }
        if (engine->checkKeyPress(BPK_4))
        {
            weapon_ = 4;
        }
    }
    if (engine->checkKeyPress(BPK_w) && engine->checkKeyPress(BPK_d))
    {
        r->FaceTowards = Towards_RightUp;
    }
    if (engine->checkKeyPress(BPK_s) && engine->checkKeyPress(BPK_a))
    {
        r->FaceTowards = Towards_LeftDown;
    }
    //实际的朝向可以不能走到
    if (man_x1_ != r->X1 || man_y1_ != r->Y1)
    {
        r->TowardsX1 = man_x1_ - r->X1;
        r->TowardsY1 = man_y1_ - r->Y1;
    }

    if (canWalk90(man_x1_, man_y1_))
    {
        r->X1 = man_x1_;
        r->Y1 = man_y1_;
    }

    if (r->CoolDown == 0)
    {
        if (r->PhysicalPower >= 20 && engine->checkKeyPress(BPK_j))    //轻攻击
        {
            r->CoolDown = 60;
            r->ActType = weapon_;
            r->ActFrame = 0;
            r->PhysicalPower -= 5;
            r->ActType2 = 0;
        }
        if (r->PhysicalPower >= 30 && engine->checkKeyPress(BPK_i))    //重攻击
        {
            r->CoolDown = 120;
            r->ActType = weapon_;
            r->ActFrame = 0;
            r->PhysicalPower -= 10;
            r->ActType2 = 1;
        }
        if (r->PhysicalPower >= 10 && engine->checkKeyPress(BPK_m))    //闪身
        {
            r->CoolDown = 150;    //冷却更长，有收招硬直
            r->SpeedX1 = r->TowardsX1;
            r->SpeedY1 = r->TowardsY1;
            double norm = EuclidDis(r->SpeedX1, r->SpeedY1);
            if (norm > 0)
            {
                r->SpeedX1 *= 5.0 / norm;
                r->SpeedY1 *= 5.0 / norm;
            }
            r->SpeedFrame = 20;
            r->ActFrame = 0;
            r->ActType = 0;
            r->PhysicalPower -= 3;
            r->ActType2 = 2;
        }
        if (r->PhysicalPower >= 50 && engine->checkKeyPress(BPK_k))    //医疗
        {
            r->CoolDown = 240;
            r->ActFrame = 0;
            r->ActType = 0;
            r->PhysicalPower -= 5;
            r->ActType2 = 3;
        }
    }

}

void BattleSceneHades::dealEvent2(BP_Event& e)
{

}

void BattleSceneHades::onEntrance()
{
    calViewRegion();
    Audio::getInstance()->playMusic(info_->Music);
    //注意此时才能得到窗口的大小，用来设置头像的位置
    head_self_->setPosition(80, 100);

    addChild(MainScene::getInstance()->getWeather());

    earth_texture_ = Engine::getInstance()->createARGBRenderedTexture(COORD_COUNT * TILE_W * 2, COORD_COUNT * TILE_H * 2);

    readBattleInfo();
    //初始状态
    for (auto r : battle_roles_)
    {
        setRoleInitState(r);

        auto p = pos45To90(r->X(), r->Y());

        r->X1 = p.x;
        r->Y1 = p.y;
        r->Progress = 0;
        if (r->HeadID == 0)
        {
            role_ = r;
            head_self_->setRole(r);
            man_x1_ = r->X1;
            man_y1_ = r->Y1;
        }
    }
}

void BattleSceneHades::onExit()
{
    Engine::getInstance()->destroyTexture(earth_texture_);
}

void BattleSceneHades::backRun()
{
    for (auto r : battle_roles_)
    {
        if (r->SpeedFrame > 0)
        {
            auto x = r->X1 + r->SpeedX1;
            auto y = r->Y1 + r->SpeedY1;
            if (canWalk90(x, y))
            {
                r->X1 = x;
                r->Y1 = y;
            }
            r->SpeedFrame--;
        }
        if (r->CoolDown > 0) { r->CoolDown--; }
        if (r->CoolDown == 0)
        {
            if (current_frame_ % 10 == 0) { r->PhysicalPower += 1; }
            r->PhysicalPower = GameUtil::limit(r->PhysicalPower, 0, 100);
        }
    }
    //降低计算量,每3帧内计算的东西都不同
    if (current_frame_ % 3 == 0)
    {
        int current_frame2 = current_frame_ / 3;
        for (auto r : battle_roles_)
        {
            if (r->ActType >= 0)
            {
                //音效和动画
                if (r->ActFrame == r->FightFrame[r->ActType] / 3 * 2)
                {
                    for (int i = 0; i < ROLE_MAGIC_COUNT; i++)
                    {
                        if (r->MagicID[i] > 0)
                        {
                            auto m = Save::getInstance()->getMagic(r->MagicID[i]);
                            if (m->MagicType == r->ActType)
                            {
                                Audio::getInstance()->playESound(m->SoundID);
                                AttackEffect ae;
                                ae.Attacker = r;
                                norm(r->TowardsX1, r->TowardsY1);
                                ae.X1 = r->X1 + TILE_W * 2 * r->TowardsX1;
                                ae.Y1 = r->Y1 + TILE_H * 2 * r->TowardsY1;
                                ae.Path = fmt1::format("eft/eft{:03}", m->EffectID);
                                ae.Frame = 0;
                                ae.Heavy = r->ActType2;
                                attack_effects_.push_back(std::move(ae));
                                //break;
                            }
                        }

                    }

                    //Audio::getInstance()->playESound(r->ActType);
                }
                //动作帧数计算
                if (r->ActFrame >= r->FightFrame[r->ActType] - 2)
                {
                    if (r->CoolDown == 0)
                    {
                        r->ActType = -1;
                        r->ActFrame = 0;
                    }
                    else
                    {
                        r->ActFrame = r->FightFrame[r->ActType] - 2;    //cooldown结束前停在最后行动帧，最后一帧一般是无行动的图
                    }
                }
                else
                {
                    if (r->ActType2 == 1)
                    {
                        if (current_frame2 % 4 == 0)
                        {
                            r->ActFrame++;
                            if (r->ActFrame >= 7)
                            {
                                x_ = rng_.rand_int(2) - rng_.rand_int(2);
                                y_ = rng_.rand_int(2) - rng_.rand_int(2);
                            }
                        }
                    }
                    else
                    {
                        r->ActFrame++;
                    }
                }
            }
            else
            {
                r->ActFrame = 0;
            }
        }
        //删除播放完毕的
        if (!attack_effects_.empty() && attack_effects_[0].Frame >= TextureManager::getInstance()->getTextureGroupCount(attack_effects_[0].Path))
        {
            attack_effects_.pop_back();
        }
        if (current_frame2 % 2 == 0)
        {
            for (auto& ae : attack_effects_)
            {
                if (ae.Heavy)
                {
                    if (current_frame2 % 4 == 0)
                    {
                        ae.Frame++;
                    }
                }
                else
                {
                    ae.Frame++;
                }
            }
        }
    }
    else if (current_frame_ % 3 == 1)
    {
        
    }
}