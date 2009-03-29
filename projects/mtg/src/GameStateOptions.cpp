#include "../include/config.h"
#include "../include/GameStateOptions.h"
#include "../include/GameApp.h"
#include "../include/OptionItem.h"
#include "../include/SimpleMenu.h"
#include "../include/GameOptions.h"

GameStateOptions::GameStateOptions(GameApp* parent): GameState(parent) {
  optionsList = NULL;
  optionsMenu = NULL;
}


GameStateOptions::~GameStateOptions() {

}

void GameStateOptions::Start()
{

  timer =  0;
  mState = SHOW_OPTIONS;
  JRenderer::GetInstance()->ResetPrivateVRAM();
  JRenderer::GetInstance()->EnableVSync(true);

  optionsList = NEW OptionsList();
  if (GameApp::HasMusic) optionsList->Add(NEW OptionItem(OPTIONS_MUSICVOLUME, "Music volume", 100, 10));
  optionsList->Add(NEW OptionItem(OPTIONS_SFXVOLUME, "SFX volume", 100, 10));
  // WALDORF - added next line
  optionsList->Add(NEW OptionItem(OPTIONS_INTERRUPT_SECONDS, "Seconds to pause for an Interrupt", 20, 1));
  if (GameOptions::GetInstance()->values[OPTIONS_DIFFICULTY_MODE_UNLOCKED].getIntValue()) {
    optionsList->Add(NEW OptionItem(OPTIONS_DIFFICULTY, "Difficulty", 3, 1));
  }
  JLBFont * mFont = GameApp::CommonRes->GetJLBFont("graphics/f3");
  optionsMenu = NEW SimpleMenu(102, this,mFont, 50,170);
  optionsMenu->Add(1, "Save & Back to Main Menu");
  optionsMenu->Add(2, "Back to Main Menu");
  optionsMenu->Add(3, "Cancel");

}


void GameStateOptions::End()
{
  JRenderer::GetInstance()->EnableVSync(false);
  SAFE_DELETE(optionsList);
  SAFE_DELETE(optionsMenu);
}


void GameStateOptions::Update(float dt)
{

  timer+= dt;
  if (mState == SHOW_OPTIONS){
    if (PSP_CTRL_START == mEngine->ReadButton()){
      mState = SHOW_OPTIONS_MENU;
    }

    optionsList->Update(dt);
  }else{
    optionsMenu->Update(dt);
  }


}


void GameStateOptions::Render()
{
  //Erase
  JRenderer::GetInstance()->ClearScreen(ARGB(0,0,0,0));

  const char * const CreditsText[] = {
    "Wagic, The Homebrew ?! by WilLoW",
    "",
    "updates, new cards, and more on http://www.wololo.net/wagic",
    "Many thanks to the devs and card creators who help this project",
    "",
    "Developped with the JGE++ Library (http://jge.khors.com)",
    "Player's avatar from http://mathieuchoinet.blogspot.com, under CC License",
    "Background picture from KDE4 , www.kde.org",
    "SFX From www.soundsnap.com",
    "",
    "Music by Celestial Aeon Project, under Creative Commons License",
    "Their music can be downloaded at http://www.jamendo.com",
    "",
    "This work is not related to or endorsed by Wizards of the Coast, Inc",
    "",
    "Please support this project with donations at http://wololo.net/wagic",
  };



  JLBFont * mFont = GameApp::CommonRes->GetJLBFont("graphics/magic");
  mFont->SetColor(ARGB(255,200,200,200));
  mFont->SetScale(1.0);
  float startpos = 272 - timer * 10;
  float pos = startpos;
  int size = sizeof(CreditsText) / sizeof(CreditsText[0]);
  for (int i = 0; i < size; i++){
    pos = startpos +20*i;
    if (pos > -20){
      mFont->DrawString(CreditsText[i],SCREEN_WIDTH/2,pos ,JGETEXT_CENTER);
    }
  }

  if (pos < -20) timer = 0;
  mFont->SetScale(1.f);

  if (mState == SHOW_OPTIONS_MENU){
    optionsMenu->Render();
  }

  optionsList->Render();

}


void GameStateOptions::ButtonPressed(int controllerId, int controlId)
{
  switch (controlId){
  case 1:
    optionsList->save();
  case 2:
    mParent->SetNextState(GAME_STATE_MENU);
    break;
  case 3:
    mState = SHOW_OPTIONS;
    break;
  }
};


