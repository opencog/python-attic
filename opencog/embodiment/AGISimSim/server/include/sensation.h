/***************************************************************************
 *  Sensation classes.        
 *
 *  Project: AgiSim
 *
 *  See implementation (.cc, .cpp) files for license details (GPL).
 *
 *  Fri Feb 18 11:35:16 2005
 *  Copyright  2005  Ari A. Heljakka / Novamente LLC
 *  Email [heljakka at iki dot fi]
 *																			
 *	19.01.06	FP	formatting 
 ****************************************************************************/
 
#ifndef SENSATION_H
#define SENSATION_H

#include "XMLNode.h"
#include "remoteobject.h"

//------------------------------------------------------------------------------------------------------------
/** @class Sensation
	\brief The base class for taste (later: for other sensations, too)*/
//------------------------------------------------------------------------------------------------------------
struct Sensation : public RemoteObject {
	Sensation ();
	Sensation (XMLNode& node);	
	Sensation (int _i, int _q);
};

//------------------------------------------------------------------------------------------------------------
/** @class CustomSensation
	\brief A single sensation of an unspecified modality.

	Custom-type sensations can be used to express sensations without
	any obvious type. In homo sapiens these are known as "feelings".
	For agents, we use them eg. for diagnostics. */
//------------------------------------------------------------------------------------------------------------
struct CustomSensation : public Sensation {
	CustomSensation () {}
	CustomSensation (string _name, int _i, int _q)
					: Sensation(_q,_i), name(_name) {}
	string name;
	std::string AsXML() const;
};

//------------------------------------------------------------------------------------------------------------
/** @class Taste
	\brief A single taste sensation source. */
//------------------------------------------------------------------------------------------------------------
struct Taste : public Sensation {
	Taste () {}
	Taste (int _i, int _q) : Sensation(_q,_i) {}
	Taste (XMLNode& node);		
		
	std::string AsXML() const;	
};

//------------------------------------------------------------------------------------------------------------
/** @class Field
	\brief The base class for smell and sound sensations. */
//------------------------------------------------------------------------------------------------------------
class IntMaintainer : public Listener {
	int& target;
public:
	IntMaintainer (int& _target) : target(_target) { }
	virtual void OnUpdate(const void* new_val)	{
		target = atoi((char*)new_val);
		 LOG("IntMaintainer", 3, "Target to "+toString(target));
	}
};

//------------------------------------------------------------------------------------------------------------
/** @class Field
	\brief A base class of fading-with-distance sensations. */
//------------------------------------------------------------------------------------------------------------
class Field : public RemoteObject { //public iXMLvalue 
protected:
public:
	Field ();
	virtual ~Field() {}
		
	std::string  source;	
		
	int  VolumeAt (double x, double y,double z, int volumeFadePerDistance, std::string wIntensityPropertyName) const;
};

//------------------------------------------------------------------------------------------------------------
/** @class Sound
	\brief A single sound source. */
//------------------------------------------------------------------------------------------------------------
class Sound : public Field {
public:
	Sound ();
	Sound (int _volume, int _quality, int _duration);
	Sound (std::string _source, int _volume, int _freq, int _duration=0);
	Sound (std::string _source, XMLNode& node);
	virtual ~Sound() {}

	std::string AsXML() const;
};

//------------------------------------------------------------------------------------------------------------
/** @class Smell
	\brief A single smell source. */
//------------------------------------------------------------------------------------------------------------
class Smell : public Field {
public:
	Smell ();
	Smell (int _volume, int _quality);
	Smell (std::string _source, int _volume, int _quality);
	Smell (std::string _source, XMLNode& node);
	virtual ~Smell() {}
		
	std::string AsXML() const;
};

//------------------------------------------------------------------------------------------------------------
/** @class Proprioception
	\brief The agent's sensations about its internal state. */
//------------------------------------------------------------------------------------------------------------
struct Proprioception : public RemoteObject { //public iXMLvalue
	int  energy;
	Proprioception (int _energy);
	std::string  AsXML() const;
};

//------------------------------------------------------------------------------------------------------------
/** @class WorldObjectProperty
	\brief The perceptual non-visual properties of the object.
	The visual properties are determined by the corresponding mesh file.*/
//------------------------------------------------------------------------------------------------------------
struct WorldObjectProperty : public RemoteObject {
	WorldObjectProperty();

	vector<Sound>  sound;
	Smell 		   smell;
	Taste 		   taste;
	
	string AsXML() const;
};

//------------------------------------------------------------------------------------------------------------
/** @class MapInfoObjectProperty
	\brief The perceptual non-visual properties of the object.
	The visual properties are determined by the corresponding mesh file.*/
//------------------------------------------------------------------------------------------------------------
struct MapInfoObjectProperty : public RemoteObject {
	MapInfoObjectProperty();
	std::string objName;
        float px,py,pz;	
        float rx,ry,rz;	
	bool edible;
	bool drinkable;
	string AsXML() const;
};

#endif
