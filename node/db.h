// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "../core/common.h"
#include "../core/block_crypt.h"
#include "../sqlite/sqlite3.h"

namespace beam {

class NodeDB
{
public:

	struct StateFlags {
		static const uint32_t Functional	= 0x1;	// has block body
		static const uint32_t Reachable		= 0x2;	// has only functional nodes up to the genesis state
		static const uint32_t Active		= 0x4;	// part of the current blockchain
	};

	struct ParamID {
		enum Enum {
			DbVer,
			CursorRow,
			CursorHeight,
			FossilHeight,
			CfgChecksum,
			MyID,
			SyncTarget,
			LoHorizon,
		};
	};

	struct Query
	{
		enum Enum
		{
			Begin,
			Commit,
			Rollback,
			Scheme,
			ParamGet,
			ParamIns,
			ParamUpd,
			StateIns,
			StateDel,
			StateGet,
			StateGetHash,
			StateGetHeightAndPrev,
			StateFind,
			StateFind2,
			StateFindWorkGreater,
			StateUpdPrevRow,
			StateGetNextFCount,
			StateSetNextCount,
			StateSetNextCountF,
			StateGetHeightAndAux,
			StateGetNextFunctional,
			StateSetFlags,
			StateGetFlags0,
			StateGetFlags1,
			StateGetChainWork,
			StateGetNextCount,
			StateSetPeer,
			StateGetPeer,
			TipAdd,
			TipDel,
			TipReachableAdd,
			TipReachableDel,
			EnumTips,
			EnumFunctionalTips,
			EnumAtHeight,
			EnumAncestors,
			StateGetPrev,
			Unactivate,
			UnactivateAll,
			Activate,
			MmrGet,
			MmrSet,
			HashForHist,
			StateGetBlock,
			StateSetBlock,
			StateDelBlock,
			StateSetRollback,
			MinedIns,
			MinedUpd,
			MinedDel,
			MinedSel,
			MacroblockEnum,
			MacroblockIns,
			MacroblockDel,
			PeerAdd,
			PeerDel,
			PeerEnum,
			BbsEnum,
			BbsEnumAll,
			BbsFind,
			BbsDelOld,
			BbsIns,
			DummyIns,
			DummyFind,
			DummyUpdHeight,
			DummyDel,
			KernelIns,
			KernelFind,
			KernelDel,
			KernelDelAll,

			Dbg0,
			Dbg1,
			Dbg2,
			Dbg3,
			Dbg4,

			count
		};
	};


	NodeDB();
	virtual ~NodeDB();

	void Close();
	void Open(const char* szPath);

	virtual void OnModified() {}

	class Recordset
	{
		sqlite3_stmt* m_pStmt;
	public:

		NodeDB & m_DB;

		Recordset(NodeDB&);
		Recordset(NodeDB&, Query::Enum, const char*);
		~Recordset();

		void Reset();
		void Reset(Query::Enum, const char*);

		// Perform the query step. SELECT only: returns true while there're rows to read
		bool Step();
		void StepStrict(); // must return at least 1 row, applicable for SELECT

		// in/out
		void put(int col, uint32_t);
		void put(int col, uint64_t);
		void put(int col, const Blob&);
		void put(int col, const char*);
		void get(int col, uint32_t&);
		void get(int col, uint64_t&);
		void get(int col, Blob&);
		void get(int col, ByteBuffer&); // don't over-use

		const void* get_BlobStrict(int col, uint32_t n);

		template <typename T> void put_As(int col, const T& x) { put(col, Blob(&x, sizeof(x))); }
		template <typename T> void get_As(int col, T& out) { out = get_As<T>(col); }
		template <typename T> const T& get_As(int col) { return *(const T*) get_BlobStrict(col, sizeof(T)); }

		void putNull(int col);
		bool IsNull(int col);

		void put(int col, const Merkle::Hash& x) { put_As(col, x); }
		void get(int col, Merkle::Hash& x) { get_As(col, x); }
		void put(int col, const Block::PoW& x) { put_As(col, x); }
		void get(int col, Block::PoW& x) { get_As(col, x); }
	};

	int get_RowsChanged() const;
	uint64_t get_LastInsertRowID() const;

	class Transaction {
		NodeDB* m_pDB;
	public:
		Transaction(NodeDB* = NULL);
		Transaction(NodeDB& db) :Transaction(&db) {}
		~Transaction(); // by default - rolls back

		bool IsInProgress() const { return NULL != m_pDB; }

		void Start(NodeDB&);
		void Commit();
		void Rollback();
	};

	// Hi-level functions

	void ParamSet(uint32_t ID, const uint64_t*, const Blob*);
	bool ParamGet(uint32_t ID, uint64_t*, Blob*);

	uint64_t ParamIntGetDef(int ID, uint64_t def = 0);

	uint64_t InsertState(const Block::SystemState::Full&); // Fails if state already exists

	uint64_t StateFindSafe(const Block::SystemState::ID&);
	void get_State(uint64_t rowid, Block::SystemState::Full&);
	void get_StateHash(uint64_t rowid, Merkle::Hash&);

	bool DeleteState(uint64_t rowid, uint64_t& rowPrev); // State must exist. Returns false if there are ancestors.

	uint32_t GetStateNextCount(uint64_t rowid);
	uint32_t GetStateFlags(uint64_t rowid);
	void SetFlags(uint64_t rowid, uint32_t);

	void SetStateFunctional(uint64_t rowid);
	void SetStateNotFunctional(uint64_t rowid);

	void set_Peer(uint64_t rowid, const PeerID*);
	bool get_Peer(uint64_t rowid, PeerID&);

	void SetStateBlock(uint64_t rowid, const Blob& bodyP, const Blob& bodyE);
	void GetStateBlock(uint64_t rowid, ByteBuffer* pP, ByteBuffer* pE, ByteBuffer* pRollback);
	void SetStateRollback(uint64_t rowid, const Blob& rollback);
	void DelStateBlockPRB(uint64_t rowid); // perishable and rollback, but no ethernal
	void DelStateBlockAll(uint64_t rowid);

	struct StateID {
		uint64_t m_Row;
		Height m_Height;
		void SetNull();
	};

	void get_StateID(const StateID&, Block::SystemState::ID&);

	struct WalkerState {
		Recordset m_Rs;
		StateID m_Sid;

		WalkerState(NodeDB& db) :m_Rs(db) {}
		bool MoveNext();
	};

	void EnumTips(WalkerState&); // height lowest to highest
	void EnumFunctionalTips(WalkerState&); // chainwork highest to lowest

	void EnumStatesAt(WalkerState&, Height);
	void EnumAncestors(WalkerState&, const StateID&);
	bool get_Prev(StateID&);
	bool get_Prev(uint64_t&);

	bool get_Cursor(StateID& sid);

    void get_Proof(Merkle::IProofBuilder&, const StateID& sid, Height hPrev);
    void get_PredictedStatesHash(Merkle::Hash&, const StateID& sid); // For the next block.

	void get_ChainWork(uint64_t, Difficulty::Raw&);

	// the following functions move the curos, and mark the states with 'Active' flag
	void MoveBack(StateID&);
	void MoveFwd(const StateID&);

	void assert_valid(); // diagnostic, for tests only

	void SetMined(const StateID&, const Amount&);
	bool DeleteMinedSafe(const StateID&);

	struct WalkerMined {
		Recordset m_Rs;
		StateID m_Sid;
		Amount m_Amount;

		WalkerMined(NodeDB& db) :m_Rs(db) {}
		bool MoveNext();
	};

	void EnumMined(WalkerMined&, Height hMin); // from low to high

	void EnumMacroblocks(WalkerState&); // highest to lowest
	void MacroblockIns(uint64_t rowid);
	void MacroblockDel(uint64_t rowid);

	struct WalkerPeer
	{
		Recordset m_Rs;

		struct Data {
			PeerID m_ID;
			uint32_t m_Rating;
			uint64_t m_Address;
			Timestamp m_LastSeen;
		} m_Data;

		WalkerPeer(NodeDB& db) :m_Rs(db) {}
		bool MoveNext();
	};

	void EnumPeers(WalkerPeer&); // highest to lowest
	void PeerIns(const WalkerPeer::Data&);
	void PeersDel();

	struct WalkerBbs
	{
		Recordset m_Rs;

		struct Data {
			ECC::Hash::Value m_Key;
			BbsChannel m_Channel;
			Timestamp m_TimePosted;
			Blob m_Message;
		} m_Data;

		WalkerBbs(NodeDB& db) :m_Rs(db) {}
		bool MoveNext();
	};

	void EnumBbs(WalkerBbs&); // set channel and min time before invocation
	void EnumAllBbs(WalkerBbs&); // ordered by Channel,Time.
	void BbsIns(const WalkerBbs::Data&); // must be unique (if not sure - first try to find it)
	bool BbsFind(WalkerBbs&); // set Key
	void BbsDelOld(Timestamp tMinToRemain);

	void InsertDummy(Height h, const Blob&);
	uint64_t FindDummy(Height& h, Blob&);
	void DeleteDummy(uint64_t);
	void SetDummyHeight(uint64_t, Height);

	void InsertKernel(const Blob&, Height h);
	void DeleteKernel(const Blob&, Height h);
	Height FindKernel(const Blob&); // in case of duplicates - returning the one with the largest Height

	uint64_t FindStateWorkGreater(const Difficulty::Raw&);

	// reset cursor to zero. Keep all the data: Mined, local macroblocks, peers, bbs, dummy UTXOs
	void ResetCursor();

private:

	sqlite3* m_pDb;
	sqlite3_stmt* m_pPrep[Query::count];

	void TestRet(int);
	void ThrowSqliteError(int);
	static void ThrowError(const char*);
	static void ThrowInconsistent();

	void Create();
	void ExecQuick(const char*);
	bool ExecStep(sqlite3_stmt*);
	bool ExecStep(Query::Enum, const char*); // returns true while there's a row

	sqlite3_stmt* get_Statement(Query::Enum, const char*);


	void TipAdd(uint64_t rowid, Height);
	void TipDel(uint64_t rowid, Height);
	void TipReachableAdd(uint64_t rowid);
	void TipReachableDel(uint64_t rowid);
	void SetNextCount(uint64_t rowid, uint32_t);
	void SetNextCountFunctional(uint64_t rowid, uint32_t);
	void OnStateReachable(uint64_t rowid, uint64_t rowPrev, Height, bool);
	void BuildMmr(uint64_t rowid, uint64_t rowPrev, Height);
	void put_Cursor(const StateID& sid); // jump

	void TestChanged1Row();

	struct Dmmr;
};



} // namespace beam
