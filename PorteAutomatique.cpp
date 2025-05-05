#include "PorteAutomatique.h"

PorteAutomatique::PorteAutomatique(int p1, int p2, int p3, int p4, float& distanceRef)
    : _stepper(AccelStepper::FULL4WIRE, p1, p3, p2, p4),
      _distance(distanceRef), _etat(FERMEE),
      _angleOuvert(90), _angleFerme(0), _stepsPerRev(2048),
      _distanceOuverture(20), _distanceFermeture(30)
{
    _stepper.setMaxSpeed(500);
    _stepper.setAcceleration(200);
    _stepper.setCurrentPosition(_angleEnSteps(_angleFerme)); // Position initiale = fermée
}

void PorteAutomatique::update() {
    _currentTime = millis();
    _mettreAJourEtat();
    _stepper.run();
}

void PorteAutomatique::_mettreAJourEtat() {
    switch (_etat) {
        case FERMEE:
            _fermeState();
            break;
        case OUVERTE:
            _ouvertState();
            break;
        case EN_OUVERTURE:
            _ouvertureState();
            break;
        case EN_FERMETURE:
            _fermetureState();
            break;
    }
}

void PorteAutomatique::_fermeState() {
    if (_distance <= _distanceOuverture) {
        _etat = EN_OUVERTURE;
        _ouvrir();
    }
}

void PorteAutomatique::_ouvertState() {
    if (_distance >= _distanceFermeture) {
        _etat = EN_FERMETURE;
        _fermer();
    }
}

void PorteAutomatique::_ouvertureState() {
    if (_stepper.distanceToGo() == 0) {
        _etat = OUVERTE;
    }
}

void PorteAutomatique::_fermetureState() {
    if (_stepper.distanceToGo() == 0) {
        _etat = FERMEE;
    }
}

void PorteAutomatique::_ouvrir() {
    _stepper.moveTo(_angleEnSteps(_angleOuvert));
}

void PorteAutomatique::_fermer() {
    _stepper.moveTo(_angleEnSteps(_angleFerme));
}

long PorteAutomatique::_angleEnSteps(float angle) const {
    return static_cast<long>((angle * _stepsPerRev) / 360.0);
}

void PorteAutomatique::setPasParTour(int pas) {
    _stepsPerRev = pas;
}

void PorteAutomatique::setAngleOuvert(float angle) {
    _angleOuvert = angle;
}

void PorteAutomatique::setAngleFerme(float angle) {
    _angleFerme = angle;
}

const char* PorteAutomatique::getEtatTexte() const {
    switch (_etat) {
        case FERMEE:
            return "FERMEE";
        case OUVERTE:
            return "OUVERTE";
        case EN_OUVERTURE:
            return "OUVERTURE";
        case EN_FERMETURE:
            return "FERMETURE";
        default:
            return "INCONNU";
    }
}
