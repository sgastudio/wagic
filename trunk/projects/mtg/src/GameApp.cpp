#include "../include/config.h"
#include <JGE.h>
#include <JRenderer.h>
#if defined (WIN32) || defined (LINUX)
#else
#include <pspfpu.h>
#endif


#include "../include/GameApp.h"
#include "../include/Subtypes.h"
#include "../include/GameStateDeckViewer.h"
#include "../include/GameStateMenu.h"
#include "../include/GameStateDuel.h"
#include "../include/GameStateOptions.h"
#include "../include/GameStateShop.h"
#include "../include/DeckStats.h"
#include "../include/Translate.h"

const char * const GameState::menuTexts[]= {"--NEW--","Deck 1", "Deck 2", "Deck 3", "Deck 4", "Deck 5", "Deck 6"} ;
JResourceManager* GameApp::CommonRes = NEW JResourceManager();
hgeParticleSystem* GameApp::Particles[] = {NULL,NULL,NULL,NULL,NULL,NULL};
int GameApp::HasMusic = 1;
JMusic * GameApp::music = NULL;
string GameApp::systemError = "";

JQuad* manaIcons[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

GameState::GameState(GameApp* parent): mParent(parent)
{
  mEngine = JGE::GetInstance();
}


GameApp::GameApp(): JApp()
{
  mScreenShotCount = 0;

  for (int i=0; i < MAX_STATE	; i++)
    mGameStates[i] = NULL;

  mShowDebugInfo = false;
  players[0] = 0;
  players[1] = 0;
  gameType = GAME_TYPE_CLASSIC;

}


GameApp::~GameApp()
{
}


void GameApp::Create()
{
#if defined (WIN32)
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#elif not defined (LINUX)
  pspfpu_set_enable(0); //disable FPU Exceptions until we find where the FPU errors come from
#endif
  //_CrtSetBreakAlloc(368);
  LOG("starting Game");

  //Link this to our settings manager.
  options.theGame = this;

  //Test for Music files presence
  std::ifstream file(RESPATH"/sound/Track0.mp3");
  if (file)
    file.close();
  else
    HasMusic = 0;
  std::ifstream file2(RESPATH"/sound/Track1.mp3");
  if (file2)
    file2.close();
  else
    HasMusic = 0;

  CommonRes->CreateTexture("graphics/menuicons.png");
  //Creating thes quad in this specific order allows us to have them in the correct order to call them by integer id
  CommonRes->CreateQuad("c_artifact", "graphics/menuicons.png", 2 + 6*36, 38, 32, 32);
  CommonRes->CreateQuad("c_green", "graphics/menuicons.png", 2 + 0*36, 38, 32, 32);
  CommonRes->CreateQuad("c_blue", "graphics/menuicons.png", 2 + 1*36, 38, 32, 32);
  CommonRes->CreateQuad("c_red", "graphics/menuicons.png", 2 + 3*36, 38, 32, 32);
  CommonRes->CreateQuad("c_black", "graphics/menuicons.png", 2 + 2*36, 38, 32, 32);
  CommonRes->CreateQuad("c_white", "graphics/menuicons.png", 2 + 4*36, 38, 32, 32);
  CommonRes->CreateQuad("c_land", "graphics/menuicons.png", 2 + 5*36, 38, 32, 32);
  manaIcons[Constants::MTG_COLOR_ARTIFACT] = GameApp::CommonRes->GetQuad("c_artifact");
  manaIcons[Constants::MTG_COLOR_LAND] = GameApp::CommonRes->GetQuad("c_land");
  manaIcons[Constants::MTG_COLOR_WHITE] = GameApp::CommonRes->GetQuad("c_white");
  manaIcons[Constants::MTG_COLOR_RED] = GameApp::CommonRes->GetQuad("c_red");
  manaIcons[Constants::MTG_COLOR_BLACK] = GameApp::CommonRes->GetQuad("c_black");
  manaIcons[Constants::MTG_COLOR_BLUE] = GameApp::CommonRes->GetQuad("c_blue");
  manaIcons[Constants::MTG_COLOR_GREEN] = GameApp::CommonRes->GetQuad("c_green");
  for (int i = sizeof(manaIcons)/sizeof(manaIcons[0]) - 1; i >= 0; --i) manaIcons[i]->SetHotSpot(16,16);


  CommonRes->CreateTexture("sets/back.jpg");
  CommonRes->CreateQuad("back", "sets/back.jpg", 0, 0, 200, 285);
  CommonRes->GetQuad("back")->SetHotSpot(100, 145);

  CommonRes->CreateTexture("sets/back_thumb.jpg");
  CommonRes->CreateQuad("back_thumb", "sets/back_thumb.jpg", 0, 0, MTG_MINIIMAGE_WIDTH, MTG_MINIIMAGE_HEIGHT);

  CommonRes->CreateTexture("graphics/particles.png");
  CommonRes->CreateQuad("particles", "graphics/particles.png", 0, 0, 32, 32);
  CommonRes->GetQuad("particles")->SetHotSpot(16,16);

  CommonRes->CreateQuad("stars", "graphics/particles.png", 64, 0, 32, 32);
  CommonRes->GetQuad("stars")->SetHotSpot(16,16);

  CommonRes->LoadJLBFont("graphics/simon",11);
  CommonRes->GetJLBFont("graphics/simon")->SetTracking(-1);
  CommonRes->LoadJLBFont("graphics/f3",16);
  CommonRes->LoadJLBFont("graphics/magic",16);


  CommonRes->CreateTexture("graphics/phasebar.png");
  CommonRes->CreateTexture("graphics/wood.png");
  CommonRes->CreateTexture("graphics/gold.png");
  CommonRes->CreateTexture("graphics/goldglow.png");
  CommonRes->CreateTexture("graphics/back.jpg");

  CommonRes->CreateTexture("graphics/handback.png");

  CommonRes->CreateTexture("sets/red.jpg");
  CommonRes->CreateTexture("sets/white.jpg");
  CommonRes->CreateTexture("sets/blue.jpg");
  CommonRes->CreateTexture("sets/black.jpg");
  CommonRes->CreateTexture("sets/green.jpg");
  CommonRes->CreateQuad("red",   "sets/red.jpg",   0, 0, 200, 285);
  CommonRes->CreateQuad("white", "sets/white.jpg", 0, 0, 200, 285);
  CommonRes->CreateQuad("blue",  "sets/blue.jpg",  0, 0, 200, 285);
  CommonRes->CreateQuad("black", "sets/black.jpg", 0, 0, 200, 285);
  CommonRes->CreateQuad("green", "sets/green.jpg", 0, 0, 200, 285);
  CommonRes->GetQuad("red")->SetHotSpot(100, 145);
  CommonRes->GetQuad("white")->SetHotSpot(100, 145);
  CommonRes->GetQuad("blue")->SetHotSpot(100, 145);
  CommonRes->GetQuad("black")->SetHotSpot(100, 145);
  CommonRes->GetQuad("green")->SetHotSpot(100, 145);

  CommonRes->CreateTexture("sets/red_thumb.jpg");
  CommonRes->CreateTexture("sets/white_thumb.jpg");
  CommonRes->CreateTexture("sets/blue_thumb.jpg");
  CommonRes->CreateTexture("sets/black_thumb.jpg");
  CommonRes->CreateTexture("sets/green_thumb.jpg");
  CommonRes->CreateQuad("red_thumb",   "sets/red_thumb.jpg",   0, 0, 28, 40);
  CommonRes->CreateQuad("white_thumb", "sets/white_thumb.jpg", 0, 0, 28, 40);
  CommonRes->CreateQuad("blue_thumb",  "sets/blue_thumb.jpg",  0, 0, 28, 40);
  CommonRes->CreateQuad("black_thumb", "sets/black_thumb.jpg", 0, 0, 28, 40);
  CommonRes->CreateQuad("green_thumb", "sets/green_thumb.jpg", 0, 0, 28, 40);
  CommonRes->GetQuad("red_thumb")->SetHotSpot(14, 20);
  CommonRes->GetQuad("white_thumb")->SetHotSpot(14, 20);
  CommonRes->GetQuad("blue_thumb")->SetHotSpot(14, 20);
  CommonRes->GetQuad("black_thumb")->SetHotSpot(14, 20);
  CommonRes->GetQuad("green_thumb")->SetHotSpot(14, 20);

  CommonRes->CreateTexture("graphics/BattleIcon.png");
  CommonRes->CreateTexture("graphics/DefenderIcon.png");
  CommonRes->CreateTexture("graphics/shadow.png");
  CommonRes->CreateQuad("BattleIcon", "graphics/BattleIcon.png", 0, 0, 25, 25);
  CommonRes->CreateQuad("DefenderIcon", "graphics/DefenderIcon.png", 0, 0, 24, 23);
  CommonRes->CreateQuad("shadow", "graphics/shadow.png", 0, 0, 1, 1);
  CommonRes->GetQuad("BattleIcon")->SetHotSpot(12, 12);
  CommonRes->GetQuad("DefenderIcon")->SetHotSpot(12, 12);
  CommonRes->GetQuad("shadow")->SetHotSpot(0.5, 0.5);

  //CommonRes->CreateTexture("graphics/interrupt.png");
  //CommonRes->CreateQuad("interrupt", "graphics/interrupt.png", 0, 0, 256, 128);

  collection = NEW MTGAllCards(&cache);

  Particles[0] = NEW hgeParticleSystem("graphics/particle1.psi", CommonRes->GetQuad("particles"));
  Particles[1] = NEW hgeParticleSystem("graphics/particle2.psi", CommonRes->GetQuad("particles"));
  Particles[2] = NEW hgeParticleSystem("graphics/particle3.psi", CommonRes->GetQuad("particles"));
  Particles[3] = NEW hgeParticleSystem("graphics/particle4.psi", CommonRes->GetQuad("particles"));
  Particles[4] = NEW hgeParticleSystem("graphics/particle5.psi", CommonRes->GetQuad("particles"));
  Particles[5] = NEW hgeParticleSystem("graphics/particle7.psi", CommonRes->GetQuad("particles"));

  mGameStates[GAME_STATE_DECK_VIEWER] = NEW GameStateDeckViewer(this);
  mGameStates[GAME_STATE_DECK_VIEWER]->Create();

  mGameStates[GAME_STATE_MENU] = NEW GameStateMenu(this);
  mGameStates[GAME_STATE_MENU]->Create();

  mGameStates[GAME_STATE_DUEL] = NEW GameStateDuel(this);
  mGameStates[GAME_STATE_DUEL]->Create();

  mGameStates[GAME_STATE_SHOP] = NEW GameStateShop(this);
  mGameStates[GAME_STATE_SHOP]->Create();

  mGameStates[GAME_STATE_OPTIONS] = NEW GameStateOptions(this);
  mGameStates[GAME_STATE_OPTIONS]->Create();

  //mGameStates[GAME_STATE_GAME] = NEW GameStateGAME(this);

  mCurrentState = NULL;
  mNextState = mGameStates[GAME_STATE_MENU];

  //  effect = new CardEffect();

  char buf[512];
  sprintf(buf, "size of MTGCard : %i\n" , sizeof(MTGCard));
  OutputDebugString(buf);
}


void GameApp::LoadGameStates()
{

  //mGameStates[GAME_STATE_MENU]->Create();
  //mGameStates[GAME_STATE_GAME]->Create();

}


void GameApp::Destroy()
{
  LOG("==Destroying GameApp==");
  for (int i=GAME_STATE_MENU;i<=MAX_STATE-1;i++)
    {
      if (mGameStates[i]){
	mGameStates[i]->Destroy();
	SAFE_DELETE(mGameStates[i]);
      }
    }

  for (int i= 0; i < 6; i++){
    SAFE_DELETE(Particles[i]);
  }

  if (collection){
    collection->destroyAllCards();
    SAFE_DELETE(collection);
  }
  SampleCache::DestroyInstance();
  delete(DeckStats::GetInstance());

  SAFE_DELETE(CommonRes);

  SAFE_DELETE(Subtypes::subtypesList);
  SAFE_DELETE(MtgSets::SetsList);

  SAFE_DELETE(music);
  Translator::EndInstance();

  SimpleMenu::destroy();


  LOG("==Destroying GameApp Successful==");

}



void GameApp::Update()
{

  if (systemError.size()) return;
  JGE* mEngine = JGE::GetInstance();
  if (mEngine->GetButtonState(PSP_CTRL_START) && mEngine->GetButtonClick(PSP_CTRL_TRIANGLE))
    {
      char s[80];
      sprintf(s, "ms0:/psp/photo/MTG%d.png", mScreenShotCount++);
      JRenderer::GetInstance()->ScreenShot(s);
    }
  //Exit when START and X ARE PRESSED SIMULTANEOUSLY
  if (mEngine->GetButtonState(PSP_CTRL_START) && mEngine->GetButtonState(PSP_CTRL_CROSS)){
    mEngine->End();
    return;
  }

  float dt = mEngine->GetDelta();
  if (dt > 35.0f)		// min 30 FPS ;)
    dt = 35.0f;

  if (mCurrentState != NULL)
    mCurrentState->Update(dt);

  if (mNextState != NULL)
    {
      if (mCurrentState != NULL)
	      mCurrentState->End();

      mCurrentState = mNextState;
     

#if defined (WIN32) || defined (LINUX)
#else
   /*   
    int maxLinear = ramAvailableLineareMax();
      int ram = ramAvailable();
      char buf[512];
      sprintf(buf, "Ram : linear max: %i - total : %i\n",maxLinear, ram);
      fprintf(stderr,buf);
   */
#endif
      mCurrentState->Start();

      mNextState = NULL;
    }



}


void GameApp::Render()
{
  if (systemError.size()){
    fprintf(stderr, systemError.c_str());
    JLBFont * mFont= CommonRes->GetJLBFont("graphics/simon");
    if (mFont) mFont->DrawString(systemError.c_str(),1,1);
    return;
  }
  if (mCurrentState != NULL)
    {
      mCurrentState->Render();
    }


}

void GameApp::SetNextState(int state)
{
  mNextState = mGameStates[state];
}

void GameApp::Pause(){

}

void GameApp::Resume(){

}

