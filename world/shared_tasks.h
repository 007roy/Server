#ifndef SHARED_TASKS_H
#define SHARED_TASKS_H
#include <unordered_map>
#include <vector>
#include <string>

#include "../common/servertalk.h"
#include "../common/global_tasks.h"
#include "cliententry.h"

class ClientListEntry;

struct SharedTaskMember {
	std::string name;
	ClientListEntry *cle;
	int char_id;
	bool leader;
	// TODO: monster mission stuff
	SharedTaskMember() : cle(nullptr), char_id(0), leader(false) {}
	SharedTaskMember(std::string name, ClientListEntry *cle, int char_id, bool leader)
	    : name(name), cle(cle), char_id(char_id), leader(leader)
	{
	}
};

struct SharedTaskMemberList {
	bool update; // dirty flag
	std::vector<SharedTaskMember> list;
	SharedTaskMemberList() : update(true) {}
};

class SharedTask {
public:
	SharedTask() : id(0), task_id(0), locked(false) {}
	SharedTask(int id, int task_id) : id(id), task_id(task_id), locked(false) {}
	~SharedTask() {}

	void AddMember(std::string name, ClientListEntry *cle = nullptr, int char_id = 0, bool leader = false)
	{
		members.update = true;
		members.list.push_back({name, cle, char_id, leader});
		if (leader)
			leader_name = name;
		if (char_id == 0)
			return;
		auto it = std::find(char_ids.begin(), char_ids.end(), char_id);
		if (it == char_ids.end())
			char_ids.push_back(char_id);
	}
	void MemberLeftGame(ClientListEntry *cle);
	inline const std::string &GetLeaderName() const { return leader_name; }
	inline SharedTaskMember *GetLeader() {
		auto it = std::find_if(members.list.begin(), members.list.end(), [](const SharedTaskMember &m) { return m.leader; });
		if (it != members.list.end())
			return &(*it);
		else
			return nullptr;
	}

	void SerializeMembers(SerializeBuffer &buf, bool include_leader = true) const;
	void SetCLESharedTasks();
	void InitActivities();
	bool UnlockActivities();
	inline void SetUpdated(bool in = true) { task_state.Updated = in; }

	void Save(); // save to database

	friend class SharedTaskManager;
private:
	inline void SetID(int in) { id = in; }
	inline void SetTaskID(int in) { task_id = in; }
	inline void SetAcceptedTime(int in) { task_state.AcceptedTime = in; }
	inline void SetLocked(bool in) { locked = in; }

	inline int GetAcceptedTime() const { return task_state.AcceptedTime; }
	int id; // id we have in our map
	int task_id; // ID of the task we're on
	bool locked;
	std::string leader_name;
	SharedTaskMemberList members;
	std::vector<int> char_ids; // every char id of someone to be locked out, different in case they leave/removed
	ClientTaskInformation task_state; // book keeping
};

class SharedTaskManager {
public:
	SharedTaskManager() : next_id(0) {}
	~SharedTaskManager() {}

	bool LoadSharedTaskState();
	bool LoadSharedTasks(int single_task = 0);

	bool AppropriateLevel(int id, int level) const;

	inline SharedTask *GetSharedTask(int id) {
		auto it = tasks.find(id);
		if (it != tasks.end())
			return &it->second;
		else
			return nullptr;
	}

	inline int GetTaskActivityCount(int task_id) const {
		auto it = task_information.find(task_id);
		if (it != task_information.end())
			return it->second.ActivityCount;
		else
			return 0; // hmm
	}

	// IPC packet processing
	void HandleTaskRequest(ServerPacket *pack);
	void HandleTaskZoneCreated(ServerPacket *pack);

	void Process();

private:
	int GetNextID();
	int next_id;
	std::unordered_map<int, SharedTask> tasks; // current active shared task states
	std::unordered_map<int, TaskInformation> task_information; // task info shit
};

#endif /* !SHARED_TASKS_H */
