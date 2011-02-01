#ifndef _CREDITS_H_
#define _CREDITS_H_

#include <vector>
#include <string>
#include <JGE.h>
#include "WFont.h"
#include <time.h>
#include "Player.h"

class GameApp;

using namespace std;

class CreditBonus
{
public:
    int value;
    string text;
    CreditBonus(int _value, string _text);
    void Render(float x, float y, WFont * font);
};

class Credits
{
private:
    time_t gameLength;
    int isDifficultyUnlocked(DeckStats * stats);
    int isMomirUnlocked();
    int isEvilTwinUnlocked();
    int isRandomDeckUnlocked();
    int IsMoreAIDecksUnlocked(DeckStats * stats);
    string unlockedTextureName;
    JQuadPtr GetUnlockedQuad(string texturename);
public:
    int value;
    Player * p1, *p2;
    GameApp * app;
    int showMsg;
    int unlocked;
    string unlockedString;
    vector<CreditBonus *> bonus;
    Credits();
    ~Credits();
    void compute(Player * _p1, Player * _p2, GameApp * _app);
    void Render();
    static int unlockRandomSet(bool force = false);
    static int unlockSetByName(string name);
    static int addCreditBonus(int value);
    static int addCardToCollection(int cardId, MTGDeck * collection);
    static int addCardToCollection(int cardId);
};

#endif
