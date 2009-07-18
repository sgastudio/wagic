#include "../include/config.h"
#include "../include/Logger.h"
#include "../include/ManaCost.h"
#include "../include/ManaCostHybrid.h"
#include "../include/ExtraCost.h"
#include "../include/TargetChooser.h"
#include "../include/Targetable.h"

#if defined (WIN32)

#include <windows.h>
#endif

ManaCost * ManaCost::parseManaCost(string s, ManaCost * _manaCost, MTGCardInstance * c){
  ManaCost * manaCost;
  if (_manaCost){
    manaCost = _manaCost;
  }else{
    manaCost = NEW ManaCost();
  }
  int state = 0;
  unsigned int start = 0;
  unsigned int end = 0;
  while (!s.empty() && state != -1){
    switch(state){
    case 0:
      start = s.find_first_of("{");
      if (start == string::npos){
	return manaCost;
      }else{
	state = 1;
      }
      break;
    case 1:
      end = s.find_first_of("}");
      if (end == string::npos){
	state = -1;
      }else{
	string value = s.substr(start+1,  end - 1 - start);
	if (value == "u"){
	  manaCost->add(Constants::MTG_COLOR_BLUE, 1);
	}else if (value == "b"){
	  manaCost->add(Constants::MTG_COLOR_BLACK, 1);
	}else if (value == "w"){
	  manaCost->add(Constants::MTG_COLOR_WHITE, 1);
	}else if (value == "g"){
	  manaCost->add(Constants::MTG_COLOR_GREEN, 1);
	}else if (value == "r"){
	  manaCost->add(Constants::MTG_COLOR_RED, 1);
	}else if (value == "x"){
	  manaCost->x();
	}else if (value == "t"){
    //Tap is handled outside of Manacost
	}else if (value[0] == 's'){
    //sacrifice
    OutputDebugString("Sacrifice\n");
    TargetChooserFactory tcf;
    TargetChooser * tc = NULL;
    size_t target_start = value.find("(");
    size_t target_end = value.find(")");
    if (target_start!=string::npos && target_end!=string::npos){
      string target = value.substr(target_start+1, target_end-1 - target_start);
      tc = tcf.createTargetChooser(target,c);
    }
    manaCost->addExtraCost(NEW SacrificeCost(tc));
	}else{
	  int intvalue = atoi(value.c_str());
	  int colors[2];
	  int values[2];
	  if (intvalue < 10 && value.size() > 1){
	    for (int i = 0; i < 2; i++){
	      char c = value[i];
	      if (c >='0' && c <='9'){
		      colors[i] = Constants::MTG_COLOR_ARTIFACT;
		      values[i] = c - '0';
	      }else{
		      for (int j = 0; j < Constants::MTG_NB_COLORS; j++){
		        if (c == Constants::MTGColorChars[j]){
		          colors[i] = j;
		          values[i] = 1;
		        }
		      }
	      }
	    }
	    manaCost->addHybrid(colors[0], values[0], colors[1], values[1]);
	  }else{
	    manaCost->add(Constants::MTG_COLOR_ARTIFACT, intvalue);
	  }
	}
	s = s.substr(end + 1);
	state = 0;
      }
      break;
    default:
      break;
    }
  }

  return manaCost;
}

ManaCost::ManaCost(){
  init();
}
ManaCost::ManaCost(int _cost[], int nb_elems){
  init();
  int i;
  int total = nb_elems;
  for (i = 0; i < total; i++){
    cost[_cost[i*2]] = _cost[i*2 + 1];
  }

}

ManaCost::ManaCost(ManaCost * _manaCost){
  init();
  int i;
  for (i=0; i<= Constants::MTG_NB_COLORS; i++){
    cost[i] = _manaCost->getCost(i);
  }
}

ManaCost::~ManaCost(){
  LOG("==Deleting ManaCost==");
  for (unsigned int i = 0;  i < nbhybrids ; i++){
    SAFE_DELETE(hybrids[i]);
  }
  if (!extraCostsIsCopy) {
    SAFE_DELETE(extraCosts);
  }
}

void ManaCost::x(){
  cost[Constants::MTG_NB_COLORS] = 1;
}

void ManaCost::init(){
  int i;
  for (i=0; i<= Constants::MTG_NB_COLORS; i++){
    cost[i] = 0;
  }
  nbhybrids = 0;
  extraCosts = NULL;
  extraCostsIsCopy = 0;
}


void ManaCost::copy(ManaCost * _manaCost){
  for (unsigned int i = 0; i <= Constants::MTG_NB_COLORS; i++){
    cost[i] = _manaCost->getCost(i);
  }
  for (unsigned int i = 0; i < _manaCost->nbhybrids; i++){
    hybrids[i] = NEW ManaCostHybrid((*_manaCost->hybrids[i]));
  }
  nbhybrids = _manaCost->nbhybrids;

  if (_manaCost->extraCosts){
    //TODO Deep copy ?
    if(!extraCostsIsCopy) SAFE_DELETE(extraCosts);
    extraCosts = _manaCost->extraCosts;
    extraCostsIsCopy = 1;
  }
}

int ManaCost::getCost(int color){
  return cost[color];
}

ManaCostHybrid * ManaCost::getHybridCost(unsigned int i){
  if (nbhybrids <= i) return NULL;
  return hybrids[i];
}

int ManaCost::getMainColor(){
  for (int i=0; i< Constants::MTG_NB_COLORS; i++){
    if (cost[i]) return i;
  }
  return 0;
}

int ManaCost::hasColor(int color){
  if (cost[color]) return 1;
  for (unsigned int i = 0; i < nbhybrids; i++){
    if (hybrids[i]->hasColor(color)) return 1;
  }
  return 0;
}

int ManaCost::isNull(){
  if (getConvertedCost()) return 0;
  if (extraCosts) return 0;
  return 1;
}

int ManaCost::getConvertedCost(){
  int result = 0;
  for (unsigned int i = 0; i < Constants::MTG_NB_COLORS; i++){
    result += cost[i];
  }
  for (unsigned int i = 0; i < nbhybrids; i++){
    result+= hybrids[i]->getConvertedCost();
  }
  return result;
}

int ManaCost::remove(int color, int value){
  cost[color] -= value;
  return 1;
}

int ManaCost::add(int color, int value){
  cost[color] += value;
  return 1;
}

int ManaCost::add(ManaCost * _cost){
  if(!_cost) return 0;
  for (unsigned int i = 0; i < Constants::MTG_NB_COLORS; i++){
    cost[i]+= _cost->getCost(i);
  }
  for (unsigned int i = 0; i < _cost->nbhybrids; i++){
    hybrids[nbhybrids] = NEW ManaCostHybrid((*_cost->hybrids[i]));
    nbhybrids++;
  }
  return 1;
}

string ManaCost::toString(){
  return "ManaCost - Todo";
}


int ManaCost::addHybrid(int c1, int v1, int c2, int v2){
  ManaCostHybrid * h = NEW ManaCostHybrid(c1,v1,c2,v2);
  hybrids[nbhybrids] = h;
  nbhybrids++;
  return nbhybrids;
}

int ManaCost::addExtraCost(ExtraCost * _cost){
  if (!extraCosts) extraCosts = NEW ExtraCosts();
  extraCosts->costs.push_back(_cost);
  OutputDebugString("Adding Sacrifice\n");
  return 1;
}


int ManaCost::isExtraPaymentSet(){
  if (!extraCosts) return 1;
  OutputDebugString("Checking costs\n");
  return extraCosts->isPaymentSet();
}

int ManaCost::resetExtraPayment(){
  if (!extraCosts) return 1;
  return extraCosts->reset();
}

int ManaCost::doPayExtra(){
  if (!extraCosts) return 0;
  return extraCosts->doPay(); //TODO reset ?
}

int ManaCost::setExtraCostsAction(MTGAbility * action, MTGCardInstance * card){
  if (extraCosts) extraCosts->setAction(action, card);
  return 1;
}

int ManaCost::pay(ManaCost * _cost){
  ManaCost * diff = Diff(_cost);
  for (int i=0; i < Constants::MTG_NB_COLORS; i++){
    cost[i] = diff->getCost(i);
  }
  delete diff;
  return 1;
  //TODO return 0 if can't afford the cost!
}

//return 1 if _cost can be paid with current data, 0 otherwise
int ManaCost::canAfford(ManaCost * _cost){
  ManaCost * diff = Diff(_cost);
  int positive = diff->isPositive();
  delete diff;
  if (positive){
    return 1;
  }
  return 0;
}

int ManaCost::isPositive(){
  for (int i=0; i < Constants::MTG_NB_COLORS; i++){

    if (cost[i] < 0){
      return 0;
    }
  }
  return 1;

}


void ManaCost::randomDiffHybrids(ManaCost * _cost, int diff[]){
  int _nbhybrids = _cost->nbhybrids;
  for (int i = 0; i < _nbhybrids; i++){
    ManaCostHybrid * h = _cost->hybrids[i];
    diff[h->color1 * 2 +1]-= h->value1;
  }
}

int ManaCost::tryToPayHybrids(ManaCostHybrid * _hybrids[], int _nbhybrids, int diff[]){
  if (!_nbhybrids) return 1;
  int result = 0;
  ManaCostHybrid * h = _hybrids[_nbhybrids -1];
  if (diff[h->color1 * 2 +1] >= h->value1){
    diff[h->color1 * 2 +1]-= h->value1;
    result = tryToPayHybrids(_hybrids,_nbhybrids -1, diff);
    if (result) return 1;
    diff[h->color1 * 2 +1]+= h->value1;
  }
  if (diff[h->color2 * 2 +1] >= h->value2){
    diff[h->color2 * 2 +1]-= h->value2;
    result = tryToPayHybrids(_hybrids,_nbhybrids -1, diff);
    if (result) return 1;
    diff[h->color2 * 2 +1]+= h->value2;
  }
  return 0;
}

//compute the difference between two mana costs
ManaCost * ManaCost::Diff(ManaCost * _cost){
  int diff[(Constants::MTG_NB_COLORS + 1 )* 2];
  diff[Constants::MTG_NB_COLORS * 2] = Constants::MTG_NB_COLORS;
  for (int i=0; i < Constants::MTG_NB_COLORS; i++){
    diff[i*2] = i;
    diff[i*2 +1] = cost[i] - _cost->getCost(i);
  }
  int hybridResult = tryToPayHybrids(_cost->hybrids, _cost->nbhybrids, diff);
  if (!hybridResult) randomDiffHybrids(_cost,diff);

  //Colorless mana, special case
  int colorless_idx = Constants::MTG_COLOR_ARTIFACT * 2 + 1;
  if (diff[colorless_idx] < 0){
    for (int i=0; i < Constants::MTG_NB_COLORS; i++){
      if (diff[i*2 + 1] > 0){
	      if (diff[i*2 + 1] + diff[colorless_idx] > 0){
	        diff[i*2 + 1] += diff[colorless_idx];
	        diff[colorless_idx] = 0;
	        break;
	      }else{
	        diff[colorless_idx] +=  diff[i*2 + 1];
	        diff[i*2 + 1] = 0;
	      }
      }
    }
  }

  //Cost X
  if (_cost->getCost(Constants::MTG_NB_COLORS)){
    diff[Constants::MTG_NB_COLORS * 2 + 1] = 0;
    for (int i=0; i < Constants::MTG_NB_COLORS; i++){
      if (diff[i*2 + 1] > 0){
	      diff[Constants::MTG_NB_COLORS * 2 + 1] += diff[i*2 + 1];
	      diff[i*2 + 1] = 0;
      }
    }
  }

  ManaCost * result = NEW ManaCost(diff, Constants::MTG_NB_COLORS +1);
  return result;

}

#ifdef WIN32
void ManaCost::Dump(){
  char buf[4096];
  OutputDebugString("\n===ManaCost===\n");
  for (int i=0; i<= Constants::MTG_NB_COLORS; i++){
    if (cost[i]) {
      sprintf(buf, "%c:%i - ", Constants::MTGColorChars[i],cost[i]);
      OutputDebugString(buf);
    }
  }

  for (int i=0; i< nbhybrids; i++){
      ManaCostHybrid * h = hybrids[i];

      sprintf(buf, "H:{%c:%i}/{%c:%i}", Constants::MTGColorChars[h->color1], h->value1, Constants::MTGColorChars[h->color2], h->value2);
      OutputDebugString(buf);
  }
  OutputDebugString("\n=============\n");
}

#endif

ostream& operator<<(ostream& out, const ManaCost& m)
{
  return out << "(manacost)";
}
