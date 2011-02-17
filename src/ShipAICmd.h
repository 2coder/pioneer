#ifndef _SHIPAICMD_H
#define _SHIPAICMD_H

#include "Ship.h"
#include "SpaceStation.h"
#include "Serializer.h"

class AICommand {
public:
	// This enum is solely to make the serialization work
	enum CmdName { CMD_NONE, CMD_JOURNEY, CMD_DOCK, CMD_ORBIT, CMD_FLYTO, CMD_KILL, CMD_KAMIKAZE };

	AICommand(Ship *ship, CmdName name) { m_ship = ship; m_cmdName = name; m_child = 0; }
	virtual ~AICommand() { if(m_child) delete m_child; }

	virtual bool TimeStepUpdate() = 0;
	bool ProcessChild();				// returns false if child is active

	// Serialisation functions
	static AICommand *Load(Serializer::Reader &rd);
	AICommand(Serializer::Reader &rd, CmdName name);
	virtual void Save(Serializer::Writer &wr);
	virtual void PostLoadFixup();

	// Signal functions
	virtual void OnDeleted(const Body *body) { if (m_child) m_child->OnDeleted(body); }

protected:
	CmdName m_cmdName;	
	Ship *m_ship;
	AICommand *m_child;
};

class AICmdJourney : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdJourney(Ship *ship, SBodyPath &dest) : AICommand(ship, CMD_JOURNEY) {
		m_dest = dest;
	}

	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		m_dest.Serialize(wr);
	}
	AICmdJourney(Serializer::Reader &rd) : AICommand(rd, CMD_JOURNEY) {
		SBodyPath::Unserialize(rd, &m_dest);
	}

private:
	SBodyPath m_dest;
};

class AICmdDock : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdDock(Ship *ship, SpaceStation *target) : AICommand (ship, CMD_DOCK) {
		m_target = target;
		m_path.endTime = 0;			// trigger path generation
	}

	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		wr.Int32(Serializer::LookupBody(m_target));
		m_path.Save(wr);
	}
	AICmdDock(Serializer::Reader &rd) : AICommand(rd, CMD_DOCK) {
		m_target = (SpaceStation *)rd.Int32();
		m_path.Load(rd);
	}
	virtual void PostLoadFixup() {
		AICommand::PostLoadFixup();
		m_target = (SpaceStation *)Serializer::LookupBody((size_t)m_target);
		m_path.PostLoadFixup();
	}

	virtual void OnDeleted(const Body *body) {
		if ((Body *)m_target == body) m_target = 0;
		AICommand::OnDeleted(body);
	}

private:
	SpaceStation *m_target;
	AIPath m_path;
};

class AICmdOrbit : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdOrbit(Ship *ship, Body *target, double orbitHeight) : AICommand (ship, CMD_ORBIT) {
		m_target = target;
		m_path.endTime = 0;			// trigger path generation
		m_orbitHeight = orbitHeight;
	}

	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		wr.Int32(Serializer::LookupBody(m_target));
		m_path.Save(wr);
		wr.Double(m_orbitHeight);
	}
	AICmdOrbit(Serializer::Reader &rd) : AICommand(rd, CMD_ORBIT) {
		m_target = (SpaceStation *)rd.Int32();
		m_path.Load(rd);
		m_orbitHeight = rd.Double();
	}
	virtual void PostLoadFixup() {
		AICommand::PostLoadFixup();
		m_target = Serializer::LookupBody((size_t)m_target);
		m_path.PostLoadFixup();
	}

	virtual void OnDeleted(const Body *body) {
		if ((Body *)m_target == body) m_target = 0;
		AICommand::OnDeleted(body);
	}

private:
	Body *m_target;
	AIPath m_path;
	double m_orbitHeight;
};

/*
class AICmdFlyTo : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdFlyTo(Ship *ship, Body *target) : AICommand (ship, CMD_FLYTO) {
		m_target = target;
		m_path.endTime = 0;			// trigger path generation
	}
	AICmdFlyTo(Ship *ship, Body *target, AIPath &path) : AICommand (ship, CMD_FLYTO) {
		m_target = target;
		m_path = path;
	}

	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		wr.Int32(Serializer::LookupBody(m_target));
		m_path.Save(wr);
	}
	AICmdFlyTo(Serializer::Reader &rd) : AICommand(rd, CMD_FLYTO) {
		m_target = (SpaceStation *)rd.Int32();
		m_path.Load(rd);
	}
	virtual void PostLoadFixup() {
		AICommand::PostLoadFixup();
		m_target = Serializer::LookupBody((size_t)m_target);
		m_path.PostLoadFixup();
	}

private:
	Body *m_target;
	AIPath m_path;
};
*/


class AICmdFlyTo : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdFlyTo(Ship *ship, Body *target) : AICommand (ship, CMD_FLYTO) {
		m_target = target;
		m_posoff = vector3d(0,0,500);
		m_endvel = 10;
	}
	AICmdFlyTo(Ship *ship, Body *target, AIPath &path) : AICommand (ship, CMD_FLYTO) {
		m_target = target;
		m_posoff = vector3d(0,0,0);
		m_endvel = 0;
	}

	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		wr.Int32(Serializer::LookupBody(m_target));
		wr.Vector3d(m_posoff);
		wr.Double(m_endvel);
	}
	AICmdFlyTo(Serializer::Reader &rd) : AICommand(rd, CMD_FLYTO) {
		m_target = (SpaceStation *)rd.Int32();
		vector3d m_posoff = rd.Vector3d();
		vector3d m_endvel = rd.Double();
	}
	virtual void PostLoadFixup() {
		AICommand::PostLoadFixup();
		m_target = Serializer::LookupBody((size_t)m_target);
	}

	virtual void OnDeleted(const Body *body) {
		if ((Body *)m_target == body) m_target = 0;
		AICommand::OnDeleted(body);
	}

private:
	Body *m_target;
	vector3d m_posoff;
	double m_endvel;
};




class AICmdKill : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdKill(Ship *ship, Ship *target) : AICommand (ship, CMD_KILL) {
		m_target = target;
		m_leadTime = m_evadeTime = m_closeTime = 0.0;
		m_lastVel = m_target->GetVelocity();
	}

	// don't actually need to save all this crap
	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		wr.Int32(Serializer::LookupBody(m_target));
	}
	AICmdKill(Serializer::Reader &rd) : AICommand(rd, CMD_KILL) {
		m_target = (Ship *)rd.Int32();
	}
	virtual void PostLoadFixup() {
		AICommand::PostLoadFixup();
		m_target = (Ship *)Serializer::LookupBody((size_t)m_target);
		m_leadTime = m_evadeTime = m_closeTime = 0.0;
		m_lastVel = m_target->GetVelocity();
	}

	virtual void OnDeleted(const Body *body) {
		if ((Body *)m_target == body) m_target = 0;
		AICommand::OnDeleted(body);
	}

private:
	Ship *m_target;
	
	double m_leadTime, m_evadeTime, m_closeTime;
	vector3d m_leadOffset, m_leadDrift, m_lastVel;
};

class AICmdKamikaze : public AICommand {
public:
	virtual bool TimeStepUpdate();
	AICmdKamikaze(Ship *ship, Body *target) : AICommand (ship, CMD_KAMIKAZE) {
		m_target = target;
	}

	virtual void Save(Serializer::Writer &wr) {
		AICommand::Save(wr);
		wr.Int32(Serializer::LookupBody(m_target));
	}
	AICmdKamikaze(Serializer::Reader &rd) : AICommand(rd, CMD_KAMIKAZE) {
		m_target = (Body *)rd.Int32();
	}
	virtual void PostLoadFixup() {
		AICommand::PostLoadFixup();
		m_target = Serializer::LookupBody((size_t)m_target);
	}

	virtual void OnDeleted(const Body *body) {
		if ((Body *)m_target == body) m_target = 0;
		AICommand::OnDeleted(body);
	}

private:
	Body *m_target;
};

#endif /* _SHIPAICMD_H */