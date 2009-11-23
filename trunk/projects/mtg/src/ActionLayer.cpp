#include "../include/config.h"
#include "../include/ActionLayer.h"
#include "../include/GameObserver.h"
#include "../include/Targetable.h"
#include "../include/WEvent.h"
#include <assert.h>

MTGAbility* ActionLayer::getAbility(int type){
  for (int i = 1; i < mCount; i++){
     MTGAbility * a = ((MTGAbility *)mObjects[i]);
     if (a->aType == type){
       return a;
     }
   }
  return NULL;
}

int ActionLayer::moveToGarbage(ActionElement * e){
  int i = getIndexOf(e);
  if (i != -1){
    if (isWaitingForAnswer() == e) setCurrentWaitingAction(NULL);
    e->destroy();
    mObjects.erase(mObjects.begin()+i);
    mCount--;
    garbage.push_back(e);
    return 1;
  }
  return 0;
        
}

int ActionLayer::cleanGarbage(){
  for (size_t i = 0; i < garbage.size(); ++i){
    delete(garbage[i]);
  }
  garbage.clear();
  return 1;
}

int ActionLayer::reactToClick(ActionElement * ability, MTGCardInstance * card){
  int result = ability->reactToClick(card);
  if (result) stuffHappened = 1;
  return result;
}


int ActionLayer::reactToTargetClick(ActionElement* ability, Targetable * card){
  int result = ability->reactToTargetClick(card);
  if (result) stuffHappened = 1;
  return result;
}

bool ActionLayer::CheckUserInput(u32 key){
  GameObserver * g = GameObserver::GetInstance();
  if (g->waitForExtraPayment && key == PSP_CTRL_CROSS){
    g->waitForExtraPayment = NULL;
    return 1;
  }
  if (menuObject){
    return false;
  }
  for (int i=0;i<mCount;i++){
    if (mObjects[i]!=NULL){
      ActionElement * currentAction = (ActionElement *)mObjects[i];
      if (currentAction->CheckUserInput(key)) return true;
    }
  }
  return false;
}


void ActionLayer::Update(float dt){
  stuffHappened = 0;
  if (menuObject){
    abilitiesMenu->Update(dt);
    return;
  }
  modal = 0;
  GameObserver* game = GameObserver::GetInstance();
  for (int i=mCount -1 ;i>=0;i--){
    if (mObjects[i]!= NULL){
      ActionElement * currentAction = (ActionElement *)mObjects[i];
      if (currentAction->testDestroy())
	      game->removeObserver(currentAction);
    }
  }
  int newPhase = game->getCurrentGamePhase();
  for (int i=0;i<mCount;i++){
    if (mObjects[i]!=NULL){
      ActionElement * currentAction = (ActionElement *)mObjects[i];
      currentAction->newPhase = newPhase;
      currentAction->Update(dt);
      currentAction->currentPhase = newPhase;
    }
  }

}

void ActionLayer::Render (){
  if (menuObject){
    abilitiesMenu->Render();
    return;
  }
  for (int i=0;i<mCount;i++){
    if (mObjects[i]!=NULL){
      ActionElement * currentAction = (ActionElement *)mObjects[i];
      //if (currentAction->getActivity() > 0){
      currentAction->Render();
      //return;
      //}
    }
  }
}


void ActionLayer::setCurrentWaitingAction(ActionElement * ae){
  assert(!ae || !currentWaitingAction);
  currentWaitingAction = ae;
}

TargetChooser * ActionLayer::getCurrentTargetChooser(){
 /* for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    if(currentAction->waitingForAnswer) return currentAction->tc;
  }
  return NULL;*/
  if (currentWaitingAction && currentWaitingAction->waitingForAnswer)
     return currentWaitingAction->tc;
  return NULL;
}

int ActionLayer::cancelCurrentAction(){
  ActionElement * ae = isWaitingForAnswer();
  if (!ae) return 0;
  ae->waitingForAnswer = 0; //TODO MOVE THIS IN ActionElement
  setCurrentWaitingAction(NULL);
  return 1;
}

ActionElement * ActionLayer::isWaitingForAnswer(){
  if (currentWaitingAction && currentWaitingAction->waitingForAnswer)
     return currentWaitingAction;
  return NULL;
}

int ActionLayer::stillInUse(MTGCardInstance * card){
  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    if(currentAction->stillInUse(card)) return 1;
  }
  return 0;
}

int ActionLayer::receiveEventPlus(WEvent * event){
  int result = 0;
  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    result += currentAction->receiveEvent(event);
  }
  return 0;
}

int ActionLayer::isReactingToTargetClick(Targetable * card){
  int result = 0;

  if (isWaitingForAnswer()) return -1;

  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    result += currentAction->isReactingToTargetClick(card);
  }
  return result;
}

int ActionLayer::reactToTargetClick(Targetable * card){
  int result = 0;

  ActionElement * ae = isWaitingForAnswer();
  if (ae) return reactToTargetClick(ae,card);

  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    result += currentAction->reactToTargetClick(card);
  }
  return result;
}

//TODO Simplify with only object !!!
int ActionLayer::isReactingToClick(MTGCardInstance * card){
  int result = 0;

  if (isWaitingForAnswer()) return -1;

  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    result += currentAction->isReactingToClick(card);
  }

  return result;
}

int ActionLayer::reactToClick(MTGCardInstance * card){
  int result = 0;

  ActionElement * ae = isWaitingForAnswer();
  if (ae) return reactToClick(ae,card);

  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    result += reactToClick(currentAction,card);
    if (result) return result;
  }
  return result;
}


void ActionLayer::setMenuObject(Targetable * object){
  menuObject = object;

  SAFE_DELETE(abilitiesMenu);

  abilitiesMenu = NEW SimpleMenu(10, this, Constants::MAIN_FONT, 100, 100);

  for (int i=0;i<mCount;i++){
    ActionElement * currentAction = (ActionElement *)mObjects[i];
    if (currentAction->isReactingToTargetClick(object)){
      abilitiesMenu->Add(i,currentAction->getMenuText());
    }
  }
  abilitiesMenu->Add(-1, "Cancel");
  modal = 1;
}

void ActionLayer::doReactTo(int menuIndex){

  if (menuObject){
    int controlid = abilitiesMenu->mObjects[menuIndex]->GetId();
    char buf[4096];
    sprintf(buf, "ACTIONLAYER doReact To %i\n",controlid);
    OutputDebugString(buf);
    if (controlid != -1){
      ActionElement * currentAction = (ActionElement *)mObjects[controlid];
      currentAction->reactToTargetClick(menuObject);
    }
    menuObject = 0;
  }
}

void ActionLayer::ButtonPressed(int controllerid, int controlid){
  if (controlid == -1){
  }else{
    ActionElement * currentAction = (ActionElement *)mObjects[controlid];
    currentAction->reactToTargetClick(menuObject);
  }
  menuObject = 0;

}

ActionLayer::ActionLayer(){
  menuObject = NULL; 
  abilitiesMenu = NULL; 
  stuffHappened = 0;
  currentWaitingAction = NULL;
}

ActionLayer::~ActionLayer(){
  SAFE_DELETE(abilitiesMenu);
  cleanGarbage();
}
