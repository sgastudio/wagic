/*
The Action Stack contains all information for Game Events that can be interrupted (Interruptible)
*/
#include "PrecompiledHeader.h"

#include "ActionStack.h"
#include "CardGui.h"
#include "CardSelectorSingleton.h"
#include "Damage.h"
#include "GameObserver.h"
#include "GameOptions.h"
#include "ManaCost.h"
#include "MTGAbility.h"
#include "TargetChooser.h"
#include "Translate.h"
#include "WResourceManager.h"

#include <typeinfo>

namespace
{
    float kGamepadIconSize = 0.5f;

    const std::string kInterruptString(": Interrupt");
    const std::string kNoString(": No");
    const std::string kNoToAllString(": No To All");
}

/*
NextGamePhase requested by user
*/
int NextGamePhase::resolve()
{
    GameObserver::GetInstance()->nextGamePhase();
    return 1;
}

const string NextGamePhase::getDisplayName() const
{
    std::ostringstream stream;
    stream << "NextGamePhase.  (Current phase is: " << PhaseRing::phaseName(GameObserver::GetInstance()->getCurrentGamePhase())
        << ")";

    return stream.str();
}

void NextGamePhase::Render()
{
    GameObserver * g = GameObserver::GetInstance();
    int nextPhase = (g->getCurrentGamePhase() + 1) % Constants::MTG_PHASE_CLEANUP;

    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetBase(0);
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
    char buffer[200];
    int playerId = 1;
    if (g->currentActionPlayer == GameObserver::GetInstance()->players[1])
        playerId = 2;

    sprintf(buffer, "%s %i : -> %s", _("Player").c_str(), playerId, _(PhaseRing::phaseName(nextPhase)).c_str());

    mFont->DrawString(buffer, x + 30, y, JGETEXT_LEFT);
}

NextGamePhase::NextGamePhase(int id) :
Interruptible(id)
{
    mHeight = 40;
    type = ACTION_NEXTGAMEPHASE;
}

ostream& NextGamePhase::toString(ostream& out) const
{
    out << "NextGamePhase ::: ";
    return out;
}

const string Interruptible::getDisplayName() const
{
    return typeid(*this).name();
}

float Interruptible::GetVerticalTextOffset() const
{
    static const float kTextVerticalOffset = (mHeight - WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT)->GetHeight()) / 2;
    return kTextVerticalOffset;
}

void Interruptible::Render(MTGCardInstance * source, JQuad * targetQuad, string alt1, string alt2, string action,
    bool bigQuad)
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetColor(ARGB(255,255,255,255));
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);

    mFont->DrawString(_(action).c_str(), x + 35, y + GetVerticalTextOffset(), JGETEXT_LEFT);
    JRenderer * renderer = JRenderer::GetInstance();
    JQuad * quad = WResourceManager::Instance()->RetrieveCard(source, CACHE_THUMB);
    if (!quad)
        quad = CardGui::AlternateThumbQuad(source);
    if (quad)
    {
        quad->SetColor(ARGB(255,255,255,255));
        float scale = mHeight / quad->mHeight;
        renderer->RenderQuad(quad, x + (quad->mWidth * scale / 2), y + (quad->mHeight * scale / 2), 0, scale, scale);
    }
    else if (alt1.size())
    {
        mFont->DrawString(_(alt1).c_str(), x, y + GetVerticalTextOffset());
    }

    if (bigQuad)
    {
        Pos pos = Pos(CardGui::BigWidth / 2, CardGui::BigHeight / 2 - 10, 1.0, 0.0, 220);
        CardGui::DrawCard(source, pos, CardSelectorSingleton::Instance()->GetDrawMode());
    }

    if (targetQuad)
    {
        float backupX = targetQuad->mHotSpotX;
        float backupY = targetQuad->mHotSpotY;
        targetQuad->SetColor(ARGB(255,255,255,255));
        targetQuad->SetHotSpot(targetQuad->mWidth / 2, targetQuad->mHeight / 2);
        float scale = mHeight / targetQuad->mHeight;
        renderer->RenderQuad(targetQuad, x + 150, y + ((mHeight - targetQuad->mHeight) / 2) + targetQuad->mHotSpotY, 0, scale, scale);
        targetQuad->SetHotSpot(backupX, backupY);
    }
    else if (alt2.size())
    {
        mFont->DrawString(_(alt2).c_str(), x + 120, y + GetVerticalTextOffset());
    }
}

/* Ability */
int StackAbility::resolve()
{
    return (ability->resolve());
}
void StackAbility::Render()
{
    string action = ability->getMenuText();
    MTGCardInstance * source = ability->source;
    string alt1 = source->getName();

    Targetable * _target = ability->target;
    if (ability->tc)
    {
        Targetable * t = ability->tc->getNextTarget();
        if (t)
            _target = t;
    }
    Damageable * target = NULL;
    if (_target != ability->source && (_target->typeAsTarget() == TARGET_CARD || _target->typeAsTarget()
        == TARGET_PLAYER))
    {
        target = (Damageable *) _target;
    }

    JQuad * quad = NULL;
    string alt2 = "";
    if (target)
    {
        quad = target->getIcon();
        if (target->type_as_damageable == DAMAGEABLE_MTGCARDINSTANCE)
        {
            alt2 = ((MTGCardInstance *) target)->name;
        }
    }

    Interruptible::Render(source, quad, alt1, alt2, action);
}
StackAbility::StackAbility(int id, MTGAbility * _ability) :
Interruptible(id), ability(_ability)
{
    type = ACTION_ABILITY;
}

ostream& StackAbility::toString(ostream& out) const
{
    out << "StackAbility ::: ability : " << ability;
    return out;
}

const string StackAbility::getDisplayName() const
{
    std::ostringstream stream;
    stream << "StackAbility.  (Source: " << ability->source->getDisplayName() << ")";

    return stream.str();
}

/* Spell Cast */

Spell::Spell(MTGCardInstance * _source) :
Interruptible(0)
{
    source = _source;
    mHeight = 40;
    type = ACTION_SPELL;
    cost = NEW ManaCost();
    tc = NULL;
    from = _source->getCurrentZone();
}

Spell::Spell(int id, MTGCardInstance * _source, TargetChooser * tc, ManaCost * _cost, int payResult) :
Interruptible(id), tc(tc), cost(_cost), payResult(payResult)
{
    source = _source;
    mHeight = 40;
    type = ACTION_SPELL;
    from = _source->getCurrentZone();
    for(int i = 0;i < MAX_TARGETS;i++)
    {
    if(tc && tc->targets[i] != NULL)
    _source->backupTargets[i] = tc->targets[i];
    }
}

int Spell::computeX(MTGCardInstance * card)
{
    ManaCost * c = cost->Diff(card->getManaCost());
    int x = 0;
    x = c->getCost(Constants::MTG_NB_COLORS);
    delete c;
    return x;
}

int Spell::computeXX(MTGCardInstance * card)
{
    ManaCost * c = cost->Diff(card->getManaCost());
    int xx = 0;
    xx = c->getCost(Constants::MTG_NB_COLORS) / 2;
    delete c;
    return xx;
}

bool Spell::FullfilledAlternateCost(const int &costType)
{
    bool hasFullfilledAlternateCost = false;

    switch (costType)
    {
    case ManaCost::MANA_PAID_WITH_KICKER:
        hasFullfilledAlternateCost = (payResult == ManaCost::MANA_PAID_WITH_KICKER);
        break;
    case ManaCost::MANA_PAID_WITH_ALTERNATIVE:
        hasFullfilledAlternateCost = (payResult == ManaCost::MANA_PAID_WITH_ALTERNATIVE);
        break;
    case ManaCost::MANA_PAID_WITH_BUYBACK:
        hasFullfilledAlternateCost = (payResult == ManaCost::MANA_PAID_WITH_BUYBACK);
        break;
    case ManaCost::MANA_PAID_WITH_FLASHBACK:
        hasFullfilledAlternateCost = (payResult == ManaCost::MANA_PAID_WITH_FLASHBACK);
        break;
    case ManaCost::MANA_PAID_WITH_RETRACE:
        hasFullfilledAlternateCost = (payResult == ManaCost::MANA_PAID_WITH_RETRACE);
        break;
    }

    return hasFullfilledAlternateCost;
}

const string Spell::getDisplayName() const
{
    return source->getName();
}
Spell::~Spell()
{
    SAFE_DELETE(cost);
    SAFE_DELETE(tc);
}

int Spell::resolve()
{
    GameObserver * game = GameObserver::GetInstance();
    if (!source->hasType("instant") && !source->hasType("sorcery"))
    {
        Player * p = source->controller();
        source = p->game->putInZone(source, from, p->game->battlefield);
        from = p->game->battlefield;
    }

    //Play SFX
    if (options[Options::SFXVOLUME].number > 0)
    {
        JSample * sample = source->getSample();
        if (sample)
        {
            JSoundSystem::GetInstance()->PlaySample(sample);
        }
    }

    AbilityFactory af;
    af.addAbilities(game->mLayers->actionLayer()->getMaxId(), this);
    return 1;
}

MTGCardInstance * Spell::getNextCardTarget(MTGCardInstance * previous)
{
    if (!tc)
        return NULL;
    return tc->getNextCardTarget(previous);
}
Player * Spell::getNextPlayerTarget(Player * previous)
{
    if (!tc)
        return NULL;
    return tc->getNextPlayerTarget(previous);
}
Damageable * Spell::getNextDamageableTarget(Damageable * previous)
{
    if (!tc)
        return NULL;
    return tc->getNextDamageableTarget(previous);
}
Interruptible * Spell::getNextInterruptible(Interruptible * previous, int type)
{
    if (!tc)
        return NULL;
    return tc->getNextInterruptible(previous, type);
}
Spell * Spell::getNextSpellTarget(Spell * previous)
{
    if (!tc)
        return NULL;
    return tc->getNextSpellTarget(previous);
}
Damage * Spell::getNextDamageTarget(Damage * previous)
{
    if (!tc)
        return NULL;
    return tc->getNextDamageTarget(previous);
}
Targetable * Spell::getNextTarget(Targetable * previous, int type)
{
    if (!tc)
        return NULL;
    return tc->getNextTarget(previous, type);
}

int Spell::getNbTargets()
{
    if (!tc)
        return 0;
    return tc->cursor;
}

void Spell::Render()
{
    string action = source->getName();
    string alt1 = "";

    JQuad * quad = NULL;
    string alt2 = "";
    Damageable * target = getNextDamageableTarget();
    if (target)
    {
        quad = target->getIcon();
        if (target->type_as_damageable == DAMAGEABLE_MTGCARDINSTANCE)
        {
            alt2 = ((MTGCardInstance *) target)->name;
        }
    }
    Interruptible::Render(source, quad, alt1, alt2, action, true);
}

ostream& Spell::toString(ostream& out) const
{
    out << "Spell ::: cost : " << cost;
    return out;
}

/* Put a card in graveyard */

PutInGraveyard::PutInGraveyard(int id, MTGCardInstance * _card) :
Interruptible(id)
{
    card = _card;
    removeFromGame = 0;
    type = ACTION_PUTINGRAVEYARD;
}

int PutInGraveyard::resolve()
{
    GameObserver * g = GameObserver::GetInstance();
    MTGGameZone * zone = card->getCurrentZone();
    if (zone == g->players[0]->game->inPlay || zone == g->players[1]->game->inPlay)
    {
        card->owner->game->putInZone(card, zone, card->owner->game->graveyard);
        return 1;
    }
    return 0;
}

void PutInGraveyard::Render()
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetBase(0);
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
    if (!removeFromGame)
    {
        mFont->DrawString(_("goes to graveyard").c_str(), x + 30, y, JGETEXT_LEFT);
    }
    else
    {
        mFont->DrawString(_("is exiled").c_str(), x + 30, y, JGETEXT_LEFT);
    }
    JRenderer * renderer = JRenderer::GetInstance();
    JQuad * quad = WResourceManager::Instance()->RetrieveCard(card, CACHE_THUMB);
    if (quad)
    {
        quad->SetColor(ARGB(255,255,255,255));
        float scale = 30 / quad->mHeight;
        renderer->RenderQuad(quad, x, y, 0, scale, scale);
    }
    else
    {
        mFont->DrawString(_(card->name).c_str(), x, y - 15);
    }
}

ostream& PutInGraveyard::toString(ostream& out) const
{
    out << "PutInGraveyard ::: removeFromGame : " << removeFromGame;
    return out;
}

/* Draw a Card */
DrawAction::DrawAction(int id, Player * _player, int _nbcards) :
Interruptible(id), nbcards(_nbcards), player(_player)
{
}

int DrawAction::resolve()
{
    for (int i = 0; i < nbcards; i++)
    {
        player->game->drawFromLibrary();
    }
    return 1;
}

void DrawAction::Render()
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetBase(0);
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
    char buffer[200];
    int playerId = 1;
    if (player == GameObserver::GetInstance()->players[1])
        playerId = 2;
    sprintf(buffer, _("Player %i draws %i card").c_str(), playerId, nbcards);
    mFont->DrawString(buffer, x + 35, y + GetVerticalTextOffset(), JGETEXT_LEFT);
}

ostream& DrawAction::toString(ostream& out) const
{
    out << "DrawAction ::: nbcards : " << nbcards << " ; player : " << player;
    return out;
}
//////
LifeAction::LifeAction(int id, Damageable * _target, int amount) :
Interruptible(id), amount(amount),target(_target)
{
}

int LifeAction::resolve()
{
target->life += amount;
    return 1;
}

void LifeAction::Render()
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetBase(0);
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
    char buffer[200];
    if(amount >= 0)
        sprintf(buffer, _("Player gains %i life").c_str(), amount);
    else if(amount >= 0)
        sprintf(buffer, _("Player loses %i life").c_str(), amount);
    else
        sprintf(buffer, _("Nothing happened").c_str(), amount);
    mFont->DrawString(buffer, x + 20, y, JGETEXT_LEFT);
}

ostream& LifeAction::toString(ostream& out) const
{
    out << "LifeAction ::: amount : " << amount << " ; target : " << target;
    return out;
}
/* The Action Stack itself */
int ActionStack::addPutInGraveyard(MTGCardInstance * card)
{
    PutInGraveyard * death = NEW PutInGraveyard(mCount, card);
    addAction(death);
    return 1;
}

int ActionStack::addAbility(MTGAbility * ability)
{
    StackAbility * stackAbility = NEW StackAbility(mCount, ability);
    int result = addAction(stackAbility);
    if (!game->players[0]->isAI() && ability->source->controller() == game->players[0] && 0
        == options[Options::INTERRUPTMYABILITIES].number)
        interruptDecision[0] = DONT_INTERRUPT;
    return result;
}

int ActionStack::addDraw(Player * player, int nb_cards)
{
    DrawAction * draw = NEW DrawAction(mCount, player, nb_cards);
    addAction(draw);
    GameObserver *g = GameObserver::GetInstance();
		for(int i = nb_cards; i > 0;i--)
		{
    WEvent * e = NEW WEventcardDraw(player, nb_cards);
    g->receiveEvent(e);
		}
    return 1;
}

int ActionStack::addLife(Damageable * _target, int amount)
{
    LifeAction * life = NEW LifeAction(mCount, _target, amount);
    addAction(life);
    return 1;
}

int ActionStack::addDamage(MTGCardInstance * _source, Damageable * _target, int _damage)
{
    Damage * damage = NEW Damage(_source, _target, _damage);
    addAction(damage);
    _source->thatmuch = _damage;
    _target->thatmuch = _damage;
    _target->lifeLostThisTurn += _damage;
    return 1;
}

int ActionStack::AddNextGamePhase()
{
    if (getNext(NULL, NOT_RESOLVED))
        return 0;
    NextGamePhase * next = NEW NextGamePhase(mCount);
    addAction(next);
    int playerId = 0;
    game->currentActionPlayer = game->GetInstance()->currentActionPlayer;
    if (game->currentActionPlayer == game->players[1])
    {
        playerId = 1;
    }
    interruptDecision[playerId] = 1;
    return 1;
}

int ActionStack::AddNextCombatStep()
{
    if (getNext(NULL, NOT_RESOLVED))
        return 0;
    NextGamePhase * next = NEW NextGamePhase(mCount);
    addAction(next);
    return 1;
}

int ActionStack::setIsInterrupting(Player * player)
{
    if (player == game->players[0])
    {
        interruptDecision[0] = -1;
    }
    else
    {
        interruptDecision[1] = -1;
    }
    game->isInterrupting = player;
    askIfWishesToInterrupt = NULL;
    return 1;
}

int ActionStack::addAction(Interruptible * action)
{
    for (int i = 0; i < 2; i++)
    {
        interruptDecision[i] = 0;
    }
    Add(action);
    DebugTrace("Action added to stack: " << action->getDisplayName());

    return 1;
}

Spell * ActionStack::addSpell(MTGCardInstance * _source, TargetChooser * tc, ManaCost * mana, int payResult,
    int storm)
{
    DebugTrace("ACTIONSTACK Add spell");
    if (storm > 0)
    {
        mana = NULL;
    }
    if (mana < 0)
    {
        mana = 0;
    }
    Spell * spell = NEW Spell(mCount, _source, tc, mana, payResult);
    addAction(spell);
    if (!game->players[0]->isAI() && _source->controller() == game->players[0] && 0
        == options[Options::INTERRUPTMYSPELLS].number)
        interruptDecision[0] = DONT_INTERRUPT;
    return spell;
}

Interruptible * ActionStack::getAt(int id)
{
    if (id < 0)
        id = mCount + id;
    if (id > mCount - 1)
        return NULL;
    return (Interruptible *) mObjects[id];
}

ActionStack::ActionStack(GameObserver* game)
    : game(game)
{
    for (int i = 0; i < 2; i++)
        interruptDecision[i] = 0;
    askIfWishesToInterrupt = NULL;
    timer = -1;
    currentState = -1;
    mode = ACTIONSTACK_STANDARD;
    checked = 0;

    for (int i = 0; i < 8; ++i)
    {
        std::ostringstream stream;
        stream << "iconspsp" << i;
        pspIcons[i] = WResourceManager::Instance()->RetrieveQuad("iconspsp.png", (float) i * 32, 0, 32, 32, stream.str(), RETRIEVE_MANAGE);
        pspIcons[i]->SetHotSpot(16, 16);
    }
}

int ActionStack::has(MTGAbility * ability)
{
    for (int i = 0; i < mCount; i++)
    {
        if (((Interruptible *) mObjects[i])->type == ACTION_ABILITY)
        {
            StackAbility * action = ((StackAbility *) mObjects[i]);
            if (action->state == NOT_RESOLVED && action->ability == ability)
                return 1;
        }
    }
    return 0;
}

int ActionStack::has(Interruptible * action)
{
    for (int i = 0; i < mCount; i++)
    {
        if (mObjects[i] == action)
            return 1;
    }
    return 0;
}

int ActionStack::resolve()
{
    Interruptible * action = getLatest(NOT_RESOLVED);

    if (!action)
        return 0;

    DebugTrace("Resolving Action on stack: " << action->getDisplayName());
    if (action->resolve())
    {
        action->state = RESOLVED_OK;
    }
    else
    {
        action->state = RESOLVED_NOK;
    }
    if (action->type == ACTION_DAMAGE)
        ((Damage *) action)->target->afterDamage();

    if (!getNext(NULL, NOT_RESOLVED))
    {
        for (int i = 0; i < 2; i++)
        {
					if (interruptDecision[i] != 2)
            interruptDecision[i] = 0;
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            if (interruptDecision[i] != 2)
                interruptDecision[i] = 0;
        }
    }

    return 1;

}

Interruptible * ActionStack::getPrevious(Interruptible * next, int type, int state, int display)
{
    int n = getPreviousIndex(next, type, state, display);
    if (n == -1)
        return NULL;
    return ((Interruptible *) mObjects[n]);
}

int ActionStack::getPreviousIndex(Interruptible * next, int type, int state, int display)
{
    int found = 0;
    if (!next)
        found = 1;
    for (int i = mCount - 1; i >= 0; i--)
    {
        Interruptible * current = (Interruptible *) mObjects[i];
        if (found && (type == 0 || current->type == type) && (state == 0 || current->state == state) && (display
            == -1 || current->display == display))
        {
            return i;
        }
        if (current == next)
            found = 1;
    }
    if (!found)
        return getPreviousIndex(NULL, type, state, display);
    return -1;
}

int ActionStack::count(int type, int state, int display)
{
    int result = 0;
    for (int i = 0; i < mCount; i++)
    {
        Interruptible * current = (Interruptible *) mObjects[i];
        if ((type == 0 || current->type == type) && (state == 0 || current->state == state) && (display == -1
            || current->display == display))
        {
            result++;
        }
    }
    return result;
}

Interruptible * ActionStack::getNext(Interruptible * previous, int type, int state, int display)
{
    int n = getNextIndex(previous, type, state, display);
    if (n == -1)
        return NULL;
    return ((Interruptible *) mObjects[n]);
}

int ActionStack::getNextIndex(Interruptible * previous, int type, int state, int display)
{
    int found = 0;
    if (!previous)
        found = 1;
    for (int i = 0; i < mCount; i++)
    {
        Interruptible * current = (Interruptible *) mObjects[i];
        if (found && (type == 0 || current->type == type) && (state == 0 || current->state == state) && (display
            == -1 || current->display == display))
        {
            return i;
        }
        if (current == previous)
            found = 1;
    }
    if (!found)
        return getNextIndex(NULL, type, state, display);
    return -1;
}

Interruptible * ActionStack::getLatest(int state)
{
    for (int i = mCount - 1; i >= 0; i--)
    {
        Interruptible * action = ((Interruptible *) mObjects[i]);
        if (action->state == state)
            return action;
    }
    return NULL;
}

int ActionStack::receiveEventPlus(WEvent * event)
{
    int result = 0;
    for (int i = 0; i < mCount; ++i)
    {
        Interruptible * current = (Interruptible *) mObjects[i];
        result += current->receiveEvent(event);
    }
    return result;
}

void ActionStack::Update(float dt)
{
    askIfWishesToInterrupt = NULL;
    //modal = 0;
    GameObserver * game = GameObserver::GetInstance();
    TargetChooser * tc = game->getCurrentTargetChooser();
    int newState = game->getCurrentGamePhase();
    currentState = newState;
    if (!tc)
        checked = 0;

    //Select Stack's display mode
    if (mode == ACTIONSTACK_STANDARD && tc && !checked)
    {
        checked = 1;
        for (int i = 0; i < mCount; i++)
        {
            Interruptible * current = (Interruptible *) mObjects[i];
            if (tc->canTarget(current))
            {
                if (mCurr < (int) mObjects.size() && mObjects[mCurr])
                    mObjects[mCurr]->Leaving(JGE_BTN_UP);
                current->display = 1;
                mCurr = i;
                mObjects[mCurr]->Entering();
                mode = ACTIONSTACK_TARGET;
                modal = 1;
            }
            else
            {
                current->display = 0;
            }
        }
        if (mode != ACTIONSTACK_TARGET)
        {
        }
    }
    else if (mode == ACTIONSTACK_TARGET && !tc)
    {
        mode = ACTIONSTACK_STANDARD;
        checked = 0;
    }

    if (mode == ACTIONSTACK_STANDARD)
    {
        modal = 0;
        if (getLatest(NOT_RESOLVED))
        {
            int currentPlayerId = 0;
            int otherPlayerId = 1;
            if (game->currentlyActing() != game->players[0])
            {
                currentPlayerId = 1;
                otherPlayerId = 0;
            }
            if (interruptDecision[currentPlayerId] == NOT_DECIDED)
            {
                askIfWishesToInterrupt = game->players[currentPlayerId];
                game->isInterrupting = game->players[currentPlayerId];
                modal = 1;
            }
            else if (interruptDecision[currentPlayerId] == INTERRUPT)
            {
                game->isInterrupting = game->players[currentPlayerId];

            }
            else
            {
                if (interruptDecision[otherPlayerId] == NOT_DECIDED)
                {
                    askIfWishesToInterrupt = game->players[otherPlayerId];
                    game->isInterrupting = game->players[otherPlayerId];
                    modal = 1;
                }
                else if (interruptDecision[otherPlayerId] == INTERRUPT)
                {
                    game->isInterrupting = game->players[otherPlayerId];
                }
                else
                {
                    resolve();
                }
            }
        }
    }
    else if (mode == ACTIONSTACK_TARGET)
    {
        GuiLayer::Update(dt);
    }
    if (askIfWishesToInterrupt)
    {
        // WALDORF - added code to use a game option setting to determine how
        // long the Interrupt timer should be. If it is set to zero (0), the
        // game will wait for ever for the user to make a selection.
        if (options[Options::INTERRUPT_SECONDS].number > 0)
        {
					int extraTime = 0;
					//extraTime is a multiplier, it counts the number of unresolved stack actions
					//then is used to extend the time you have to interupt.
					//this prevents you from "running out of time" while deciding.
					//before this int was added, it was possible to run out of time if you had 10 stack actions
					//and set the timer to 4 secs. BUG FIX //http://code.google.com/p/wagic/issues/detail?id=464
					extraTime = count(NULL,NOT_RESOLVED,NULL);
					if(extraTime == 0)
						extraTime = 1;//we never want this int to be 0.
            if (timer < 0)
							timer = options[Options::INTERRUPT_SECONDS].number * extraTime;
            timer -= dt;
            if (timer < 0)
                cancelInterruptOffer();
        }
    }
}

void ActionStack::cancelInterruptOffer(int cancelMode)
{
    if (game->isInterrupting == game->players[0])
    {
        interruptDecision[0] = cancelMode;
    }
    else
    {
        interruptDecision[1] = cancelMode;
    }
    askIfWishesToInterrupt = NULL;
    game->isInterrupting = NULL;
    timer = -1;
}

void ActionStack::endOfInterruption()
{
    if (game->isInterrupting == game->players[0])
    {
        interruptDecision[0] = 0;
    }
    else
    {
        interruptDecision[1] = 0;
    }
    game->isInterrupting = NULL;
}

bool ActionStack::CheckUserInput(JButton key)
{
    JButton trigger = (options[Options::REVERSETRIGGERS].number ? JGE_BTN_NEXT : JGE_BTN_PREV);
    if (mode == ACTIONSTACK_STANDARD)
    {
        if (askIfWishesToInterrupt)
        {
            if (JGE_BTN_SEC == key)
            {
                setIsInterrupting(askIfWishesToInterrupt);
                return true;
            }
            else if ((JGE_BTN_OK == key) || (trigger == key))
            {
                cancelInterruptOffer();
                return true;
            }
            else if ((JGE_BTN_PRI == key))
            {
                cancelInterruptOffer(2);
                return true;
            }
            return true;
        }
        else if (game->isInterrupting)
        {
            if (JGE_BTN_SEC == key)
            {
                endOfInterruption();
                return true;
            }
        }
    }
    else if (mode == ACTIONSTACK_TARGET)
    {
        if (modal)
        {
            if (JGE_BTN_UP == key)
            {
                if (mObjects[mCurr])
                {
                    int n = getPreviousIndex(((Interruptible *) mObjects[mCurr]), 0, 0, 1);
                    if (n != -1 && n != mCurr && mObjects[mCurr]->Leaving(JGE_BTN_UP))
                    {
                        mCurr = n;
                        mObjects[mCurr]->Entering();
                        DebugTrace("ACTIONSTACK UP TO mCurr = " << mCurr);
                    }
                }
                return true;
            }
            else if (JGE_BTN_DOWN == key)
            {
                if( mObjects[mCurr])
                {
                    int n = getNextIndex(((Interruptible *) mObjects[mCurr]), 0, 0, 1);
                    if (n!= -1 && n != mCurr && mObjects[mCurr]->Leaving(JGE_BTN_DOWN))
                    {
                        mCurr = n;
                        mObjects[mCurr]->Entering();
                        DebugTrace("ACTIONSTACK DOWN TO mCurr " << mCurr);
                    }
                }
                return true;
            }
            else if (JGE_BTN_OK == key)
            {
                DebugTrace("ACTIONSTACK CLICKED mCurr = " << mCurr);

                game->stackObjectClicked(((Interruptible *) mObjects[mCurr]));
                return true;
            }
            return true; //Steal the input to other layers if we're visible
        }
        if (JGE_BTN_CANCEL == key)
        {
            if (modal) modal = 0;
            else modal = 1;
            return true;
        }
    }
    return false;
}

//Cleans history of last turn
int ActionStack::garbageCollect()
{
    std::vector<JGuiObject *>::iterator iter = mObjects.begin();

    while (iter != mObjects.end())
    {
        Interruptible * current = ((Interruptible *) *iter);
        if (current->state != NOT_RESOLVED)
        {
            iter = mObjects.erase(iter);
            mCount--;
            SAFE_DELETE(current);
        }
        else
            ++iter;
    }
    return 1;
}

void ActionStack::Fizzle(Interruptible * action)
{
    if (!action)
    {
        DebugTrace("ACTIONSTACK ==ERROR==: action is NULL in ActionStack::Fizzle");
        return;
    }
    if (action->type == ACTION_SPELL)
    {
        Spell * spell = (Spell *) action;
        spell->source->controller()->game->putInGraveyard(spell->source);
    }
    action->state = RESOLVED_NOK;
}

void ActionStack::Render()
{
    static const float kSpacer = 8;
    static const float x0 = 250;
    static const float y0 = 0;
    float width = 200;
    float height = 90;
    float currenty = y0 + 5;

    if (mode == ACTIONSTACK_STANDARD)
    {
        if (!askIfWishesToInterrupt || !askIfWishesToInterrupt->displayStack())
            return;

        for (int i = 0; i < mCount; i++)
        {
            Interruptible * current = (Interruptible *) mObjects[i];
            if (current->state == NOT_RESOLVED)
                height += current->mHeight;
        }

        WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        mFont->SetBase(0);
        mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        mFont->SetColor(ARGB(255,255,255,255));
        JRenderer * renderer = JRenderer::GetInstance();

        //JQuad * back = WResourceManager::Instance()->GetQuad("interrupt");
        //float xScale = width / back->mWidth;
        //float yScale = height / back->mHeight;
        renderer->FillRoundRect(x0 + 16, y0 + 16, width + 2, height + 2, 10, ARGB(128,0,0,0));
        renderer->FillRoundRect(x0 - 5, y0, width + 2, height + 2, 10, ARGB(200,0,0,0));
        //renderer->RenderQuad(back,x0,y0,0,xScale, yScale);
        renderer->DrawRoundRect(x0 - 5, y0, width + 2, height + 2, 10, ARGB(255,255,255,255));

        std::ostringstream stream;
        // WALDORF - changed "interrupt ?" to "Interrupt?". Don't display count down
        // seconds if the user disables auto progressing interrupts by setting the seconds
        // value to zero in Options.

        // Mootpoint 01/12/2011: draw the interrupt text first, at the top.  Offset the rest of the 
        // unresolved stack effects down so that they don't collide with the interrupt text.
        if (options[Options::INTERRUPT_SECONDS].number == 0)
            stream << "Interrupt?";
        else
            stream << "Interrupt? " << static_cast<int>(timer);

        mFont->DrawString(stream.str(), x0 + 5, currenty);

        static const float kIconVerticalOffset = 24;
        if (mCount > 1)
        {
            renderer->RenderQuad(pspIcons[7], x0 + 10, kIconVerticalOffset, 0, kGamepadIconSize, kGamepadIconSize);
            mFont->DrawString(kInterruptString, x0 + 19, kIconVerticalOffset - 6);

            renderer->RenderQuad(pspIcons[4], x0 + 97, kIconVerticalOffset, 0, kGamepadIconSize, kGamepadIconSize);
            mFont->DrawString(kNoString, x0 + 106, kIconVerticalOffset - 6);

            renderer->RenderQuad(pspIcons[6], x0 + 145, kIconVerticalOffset, 0, kGamepadIconSize, kGamepadIconSize);
            mFont->DrawString(kNoToAllString, x0 + 154, kIconVerticalOffset - 6);
        }
        else
        {
            renderer->RenderQuad(pspIcons[7], x0 + 40, kIconVerticalOffset, 0, kGamepadIconSize, kGamepadIconSize);
            mFont->DrawString(kInterruptString, x0 + 49, kIconVerticalOffset - 6);

            renderer->RenderQuad(pspIcons[4], x0 + 140, kIconVerticalOffset - 6, 0, kGamepadIconSize, kGamepadIconSize);
            mFont->DrawString(kNoString, x0 + 146, kIconVerticalOffset - 6);
        }

        currenty += kIconVerticalOffset + kSpacer;

        for (int i = 0; i < mCount; i++)
        {
            Interruptible * current = (Interruptible *) mObjects[i];
            if (current && current->state == NOT_RESOLVED)
            {
                current->x = x0;
                current->y = currenty;
                current->Render();

                currenty += current->mHeight;
            }
        }
    }
    else if (mode == ACTIONSTACK_TARGET && modal)
    {
        for (int i = 0; i < mCount; i++)
        {
            Interruptible * current = (Interruptible *) mObjects[i];
            if (current->display)
                height += current->mHeight;
        }

        WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        mFont->SetColor(ARGB(255,255,255,255));

        JRenderer * renderer = JRenderer::GetInstance();
        renderer->FillRect(x0, y0, width, height, ARGB(200,0,0,0));
        renderer->DrawRect(x0 - 1, y0 - 1, width + 2, height + 2, ARGB(255,255,255,255));

        for (int i = 0; i < mCount; i++)
        {
            Interruptible * current = (Interruptible *) mObjects[i];
            if (mObjects[i] != NULL && current->display)
            {
                ((Interruptible *) mObjects[i])->x = x0 + 5;
                if (i != mCount - 1)
                {
                    ((Interruptible *) mObjects[i])->y = currenty;
                    currenty += ((Interruptible *) mObjects[i])->mHeight;
                }
                else
                {
                    ((Interruptible *) mObjects[i])->y = currenty + 40;
                    currenty += ((Interruptible *) mObjects[i])->mHeight + 40;
                }
                mObjects[i]->Render();
            }
        }
    }
}

#if defined (WIN32) || defined (LINUX)  || defined (IOS)

void Interruptible::Dump()
{
    string stype, sstate, sdisplay = "";
    switch (type)
    {
    case ACTION_SPELL:
        stype = "spell";
        break;
    case ACTION_DAMAGE:
        stype = "damage";
        break;
    case ACTION_DAMAGES:
        stype = "damages";
        break;
    case ACTION_NEXTGAMEPHASE:
        stype = "next phase";
        break;
    case ACTION_DRAW:
        stype = "draw";
        break;
    case ACTION_PUTINGRAVEYARD:
        stype = "put in graveyard";
        break;
    case ACTION_ABILITY:
        stype = "ability";
        break;
    default:
        stype = "unknown";
        break;
    }

    switch(state)
    {
    case NOT_RESOLVED:
        sstate = "not resolved";
        break;
    case RESOLVED_OK:
        sstate = "resolved";
        break;
    case RESOLVED_NOK:
        sstate = "fizzled";
        break;
    default:
        sstate = "unknown";
        break;
    }
    DebugTrace("type: " << stype << " " << type << " - state: " << sstate << " " << state << " - display: " << display);
}

void ActionStack::Dump()
{
    DebugTrace("=====\nDumping Action Stack=====");
    for (int i=0;i<mCount;i++)
    {
        Interruptible * current = (Interruptible *)mObjects[i];
        current->Dump();
    }
}

#endif