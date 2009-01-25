/*
 *  Wagic, The Homebrew ?! is licensed under the BSD license
 *  See LICENSE in the Folder's root
 *  http://wololo.net/wagic/
 */

#ifndef _SPELLSTACK_H_
#define _SPELLSTACK_H_

#define MAX_SPELL_TARGETS 10


#define ACTION_SPELL 10
#define ACTION_DAMAGE 11
#define ACTION_DAMAGES 12
#define ACTION_NEXTGAMEPHASE 13
#define ACTION_DRAW 14
#define ACTION_PUTINGRAVEYARD 15
#define ACTION_ABILITY 16

#define NOT_RESOLVED -2
#define RESOLVED_OK 1
#define RESOLVED_NOK -1

#include "../include/PlayGuiObject.h"
#include "GuiLayers.h"
#include "../include/TargetsList.h"
#include "../include/Targetable.h"

class GuiLayer;
class PlayGuiObject;
class MTGCardInstance;
class GameObserver;
class Player;
class Damageable;
class MTGAbility;
class Targetable;
class DamageStack;
class ManaCost;


#define ACTIONSTACK_STANDARD 0
#define ACTIONSTACK_TARGET 1

class Interruptible: public PlayGuiObject, public Targetable{
 public:
  int state, display;
  MTGCardInstance * source;
  virtual void Entering(){mHasFocus = true;};
  virtual bool Leaving(u32 key){mHasFocus = false;return true;};
  virtual bool ButtonPressed(){return true;};
  virtual int resolve(){return 0;};
  virtual void Render(){};
  int typeAsTarget(){return TARGET_STACKACTION;};
 Interruptible(int id,bool hasFocus = false):PlayGuiObject(id,40,x,y,hasFocus){state=NOT_RESOLVED;display=0;source=NULL;};
#if defined (WIN32) || defined (LINUX)
  virtual void Dump();
#endif
};

class NextGamePhase: public Interruptible {
 public:
  int resolve();
  void Render();
  NextGamePhase(int id);
};

class Spell: public Interruptible, public TargetsList {
 protected:

 public:
  ManaCost * cost;
  Spell(MTGCardInstance* _source);
  Spell(int id, MTGCardInstance* _source, Targetable * _targets[], int _nbtargets, ManaCost * _cost);
  ~Spell();
  int resolve();
  void Render();
};

class StackAbility: public Interruptible {
 public:
  MTGAbility * ability;
  int resolve();
  void Render();
  StackAbility(int id, MTGAbility * _ability);
};

class PutInGraveyard: public Interruptible {
 public:
  MTGCardInstance * card;
  int removeFromGame;
  int resolve();
  void Render();
  PutInGraveyard(int id, MTGCardInstance * _card);
};


class DrawAction: public Interruptible {
 public:
  int nbcards;
  Player * player;
  int resolve();
  void Render();
  DrawAction(int id, Player *  _player, int _nbcards);
};

class ActionStack :public GuiLayer{
 protected:
  int interruptDecision[2];
  float timer;
  int currentState;
  int mode;
  int checked;


  void unpackDamageStacks();
  void unpackDamageStack(DamageStack * ds);
  void repackDamageStacks();
 public:
  int setIsInterrupting(Player * player);
  int count( int type = 0 , int state = 0 , int display = -1);
  Interruptible * getPrevious(Interruptible * next, int type = 0, int state = 0 , int display = -1);
  int getPreviousIndex(Interruptible * next, int type = 0, int state = 0 , int display = -1);
  Interruptible * getNext(Interruptible * previous, int type = 0, int state = 0 , int display = -1);
  int getNextIndex(Interruptible * previous, int type = 0, int state = 0 , int display = -1);
  void Fizzle(Interruptible * action);
  Interruptible * _(int id);
  void cancelInterruptOffer(int cancelMode = 1);
  void endOfInterruption();
  Interruptible * getLatest(int state);
  Player * askIfWishesToInterrupt;
  int garbageCollect();
  int addAction(Interruptible * interruptible);
  int addSpell(MTGCardInstance* card, Targetable * targets[], int nbtargets, ManaCost * mana);
  int AddNextGamePhase();
  int addPutInGraveyard(MTGCardInstance * card);
  int addDraw(Player * player, int nbcards = 1);
  int addDamage(MTGCardInstance * _source, Damageable * target, int _damage);
  int addAbility(MTGAbility * ability);
  void Update(float dt);
  bool CheckUserInput(u32 key);
  virtual void Render();
  ActionStack(int id, GameObserver* _game);
  int resolve();
  int CombatDamages();
  int CombatDamages(int firststrike);
  int has(Interruptible * action);
  int has(MTGAbility * ability);
#if defined (WIN32) || defined (LINUX)
   void Dump();
#endif

};





#endif
