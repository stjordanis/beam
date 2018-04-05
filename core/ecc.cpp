#include "ecc_native.h"
#include <assert.h>

#define ENABLE_MODULE_GENERATOR
#define ENABLE_MODULE_RANGEPROOF

#pragma warning (disable: 4244) // conversion from ... to ..., possible loss of data, signed/unsigned mismatch
#pragma warning (disable: 4018) // signed/unsigned mismatch

#include "../beam/secp256k1-zkp/src/secp256k1.c"

#pragma warning (default: 4018)
#pragma warning (default: 4244)


namespace ECC {

	void SecureErase(void* p, uint32_t n)
	{
		memset(p, 0, n);
	}

	/////////////////////
	// Scalar
	const uintBig Scalar::s_Order = { // fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
		0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41
	};

	bool Scalar::IsValid() const
	{
		return m_Value < s_Order;
	}

	void Scalar::SetRandom()
	{
		// accept/reject strategy
		do
			m_Value.SetRandom();
		while (!IsValid());
	}

	void Scalar::Native::SetZero()
	{
		secp256k1_scalar_clear(this);
	}

	bool Scalar::Native::IsZero() const
	{
		return secp256k1_scalar_is_zero(this) != 0;
	}

	void Scalar::Native::SetNeg(const Native& v)
	{
		secp256k1_scalar_negate(this, &v);
	}

	void Scalar::Native::Neg()
	{
		SetNeg(*this);
	}

	bool Scalar::Native::Import(const Scalar& v)
	{
		int overflow;
		secp256k1_scalar_set_b32(this, v.m_Value.m_pData, &overflow);
		return overflow != 0;
	}

	void Scalar::Native::ImportFix(uintBig& v)
	{
		static_assert(sizeof(v) == sizeof(Scalar), "");
		while (Import((const Scalar&) v))
		{
			// overflow - better to retry (to have uniform distribution)
			Hash::Processor hp; // NoLeak?
			hp.Write(v.m_pData, sizeof(v.m_pData));
			hp.Finalize(v);
		}
	}

	void Scalar::Native::Export(Scalar& v) const
	{
		secp256k1_scalar_get_b32(v.m_Value.m_pData, this);
	}

	void Scalar::Native::Set(uint32_t v)
	{
		secp256k1_scalar_set_int(this, v);
	}

	void Scalar::Native::Set(uint64_t v)
	{
		secp256k1_scalar_set_u64(this, v);
	}

	void Scalar::Native::SetSum(const Native& a, const Native& b)
	{
		secp256k1_scalar_add(this, &a, &b);
	}

	void Scalar::Native::Add(const Native& v)
	{
		SetSum(*this, v);
	}

	void Scalar::Native::SetMul(const Native& a, const Native& b)
	{
		secp256k1_scalar_mul(this, &a, &b);
	}

	void Scalar::Native::Mul(const Native& v)
	{
		SetMul(*this, v);
	}

	void Scalar::Native::SetSqr(const Native& v)
	{
		secp256k1_scalar_sqr(this, &v);
	}

	void Scalar::Native::Sqr()
	{
		SetSqr(*this);
	}

	void Scalar::Native::SetInv(const Native& v)
	{
		secp256k1_scalar_inverse(this, &v);
	}

	void Scalar::Native::Inv()
	{
		SetInv(*this);
	}

	/////////////////////
	// Hash
	Hash::Processor::Processor()
	{
		Reset();
	}

	void Hash::Processor::Reset()
	{
		secp256k1_sha256_initialize(this);
	}

	void Hash::Processor::Write(const void* p, uint32_t n)
	{
		secp256k1_sha256_write(this, (const uint8_t*) p, n);
	}

	void Hash::Processor::Finalize(Hash::Value& v)
	{
		secp256k1_sha256_finalize(this, v.m_pData);
	}


	/////////////////////
	// Point
	const uintBig Point::s_FieldOrder = { // fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F
	};

	bool Point::Native::ImportInternal(const Point& v)
	{
		NoLeak<secp256k1_fe> nx;
		if (!secp256k1_fe_set_b32(&nx.V, v.m_X.m_pData))
			return false;

		NoLeak<secp256k1_ge> ge;
		if (!secp256k1_ge_set_xquad(&ge.V, &nx.V))
			return false;

		if (!v.m_bQuadraticResidue)
			secp256k1_fe_negate(&ge.V.y, &ge.V.y, 1);

		secp256k1_gej_set_ge(this, &ge.V);

		return true;
	}

	bool Point::Native::Import(const Point& v)
	{
		if (ImportInternal(v))
			return true;

		SetZero();
		return false;
	}

	bool Point::Native::Export(Point& v) const
	{
		if (IsZero())
		{
			memset(&v, 0, sizeof(v));
			return false;
		}

		NoLeak<secp256k1_gej> dup;
		dup.V = *this;
		NoLeak<secp256k1_ge> ge;
		secp256k1_ge_set_gej(&ge.V, &dup.V);

		secp256k1_fe_get_b32(v.m_X.m_pData, &ge.V.x);
		v.m_bQuadraticResidue = (secp256k1_gej_has_quad_y_var(this) != 0);

		return true;
	}

	void Point::Native::Import(const secp256k1_ge_storage& v)
	{
		NoLeak<secp256k1_ge> ge;
		secp256k1_ge_from_storage(&ge.V, &v);
		Import(ge.V);
	}

	void Point::Native::Import(const secp256k1_ge& v)
	{
		secp256k1_gej_set_ge(this, &v);
	}

	void Point::Native::Export(secp256k1_ge_storage& v)
	{
		NoLeak<secp256k1_ge> ge;
		secp256k1_ge_set_gej(&ge.V, this);
		secp256k1_ge_to_storage(&v, &ge.V);
	}

	void Point::Native::SetZero()
	{
		secp256k1_gej_set_infinity(this);
	}

	bool Point::Native::IsZero() const
	{
		return secp256k1_gej_is_infinity(this) != 0;
	}

	void Point::Native::SetNeg(const Native& v)
	{
		secp256k1_gej_neg(this, &v);
	}

	void Point::Native::Neg()
	{
		SetNeg(*this);
	}

	void Point::Native::SetSum(const Native& a, const Native& b)
	{
		secp256k1_gej_add_var(this, &a, &b, NULL);
	}

	void Point::Native::Add(const Native& v)
	{
		SetSum(*this, v);
	}

	void Point::Native::SetX2(const Native& v)
	{
		secp256k1_gej_double_var(this, &v, NULL);
	}

	void Point::Native::X2()
	{
		SetX2(*this);
	}

	void Point::Native::Mul(const Scalar& k)
	{
		Point::Native pt = *this;
		SetZero();

		for (uint32_t iByte = _countof(k.m_Value.m_pData); iByte--; )
		{
			uint8_t n = k.m_Value.m_pData[iByte];

			for (uint32_t iBit = 0; iBit < 8; iBit++, pt.X2())
				if (1 & (n >> iBit))
					Add(pt);
		}
	}

	/////////////////////
	// Generator
	namespace Generator
	{
		void CreatePointUntilSucceed(Point::Native& out, const uintBig& x)
		{
			Point pt;
			pt.m_X = x;
			pt.m_bQuadraticResidue = false;

			while (true)
			{
				if (out.Import(pt) && !out.IsZero())
					break;

				Hash::Processor hp;
				hp.Write(pt.m_X.m_pData, sizeof(pt.m_X));

				static const char sz[] = "point-gen-retry";
				hp.Write(sz, sizeof(sz)-1);
				hp.Finalize(pt.m_X);
			}
		}

		void GetNums(Point::Native& nums)
		{
			static const char sz[] = "The scalar for this x is unknown";
			static_assert(sizeof(sz) == sizeof(uintBig) + 1, "");

			CreatePointUntilSucceed(nums, (const uintBig&) sz);
		}

		void CreatePts(secp256k1_ge_storage* pPts, Point::Native& gpos, uint32_t nLevels)
		{
			Point::Native nums, npos, pt;

			GetNums(nums);
			nums.Add(gpos);

			npos = nums;

			for (uint32_t iLev = 1; ; iLev++)
			{
				pt = npos;

				for (uint32_t iPt = 1; ; iPt++)
				{
					pt.Export(*pPts++);

					if (iPt == nPointsPerLevel)
						break;

					pt.Add(gpos);
				}

				if (iLev == nLevels)
					break;

				for (uint32_t i = 0; i < nBitsPerLevel; i++)
					gpos.X2();

				npos.X2();
				if (iLev + 1 == nLevels)
				{
					npos.Neg();
					npos.Add(nums);
				}

			}
		}

		void SetMul(Point::Native& res, bool bSet, const secp256k1_ge_storage* pPts, uint32_t nLevels, const Scalar& k)
		{
			static_assert(8 % nBitsPerLevel == 0, "");
			const uint8_t nLevelsPerByte = 8 / nBitsPerLevel;
			static_assert(!(nLevelsPerByte & (nLevelsPerByte - 1)), "should be power-of-2");

			NoLeak<Point::Native> np;
			NoLeak<secp256k1_ge_storage> ge;

			// iterating in lsb to msb order
			for (uint32_t iByte = nLevels / nLevelsPerByte; iByte--; )
			{
				uint8_t n = k.m_Value.m_pData[iByte];

				for (uint8_t j = 0; j < nLevelsPerByte; j++, pPts += nPointsPerLevel)
				{
					uint32_t nSel = (nPointsPerLevel - 1) & n;
					n >>= nBitsPerLevel;

					/** This uses a conditional move to avoid any secret data in array indexes.
					*   _Any_ use of secret indexes has been demonstrated to result in timing
					*   sidechannels, even when the cache-line access patterns are uniform.
					*  See also:
					*   "A word of warning", CHES 2013 Rump Session, by Daniel J. Bernstein and Peter Schwabe
					*    (https://cryptojedi.org/peter/data/chesrump-20130822.pdf) and
					*   "Cache Attacks and Countermeasures: the Case of AES", RSA 2006,
					*    by Dag Arne Osvik, Adi Shamir, and Eran Tromer
					*    (http://www.tau.ac.il/~tromer/papers/cache.pdf)
					*/

					for (uint32_t i = 0; i < nPointsPerLevel; i++)
						secp256k1_ge_storage_cmov(&ge.V, pPts + i, i == nSel);

					if (bSet)
					{
						bSet = false;
						res.Import(ge.V);
					} else
					{
						// secp256k1_gej_add_ge(r, r, &add);
						np.V.Import(ge.V);
						res.Add(np.V);
					} 
				}
			}
		}

		void SetMul(Point::Native& res, bool bSet, const secp256k1_ge_storage* pPts, uint32_t nLevels, const Scalar::Native& k)
		{
			NoLeak<Scalar> k2;
			k.Export(k2.V);
			SetMul(res, bSet, pPts, nLevels, k2.V);
		}

		void HashFromSeedEx(Hash::Value& out, const char* szSeed, const char* szSeed2)
		{
			Hash::Processor hp;
			hp.Write(szSeed, strlen(szSeed));
			hp.Write(szSeed2, strlen(szSeed2));
			hp.Finalize(out);
		}

		void GetPointFromSeed(const char* szSeed, Point::Native& g)
		{
			Hash::Value hv;
			HashFromSeedEx(hv, szSeed, "generator");
			CreatePointUntilSucceed(g, hv);
		}

		void GeneratePts(const char* szSeed, secp256k1_ge_storage* pPts, uint32_t nLevels)
		{
			Point::Native g;
			GetPointFromSeed(szSeed, g);
			CreatePts(pPts, g, nLevels);
		}

		Obscured::Obscured(const char* szSeed)
		{
			Point::Native g;
			GetPointFromSeed(szSeed, g);

			Point::Native pt2 = g;
			CreatePts(m_pPts, pt2, nLevels);

			Hash::Value hv;
			HashFromSeedEx(hv, szSeed, "blind-scalar");
			m_AddScalar.ImportFix(hv);

			Generator::SetMul(pt2, true, m_pPts, nLevels, m_AddScalar); // pt2 = G * blind
			pt2.Export(m_AddPt);

			m_AddScalar.Neg();
		}

		void Obscured::SetMul(Point::Native& res, bool bSet, const Scalar::Native& k) const
		{
			if (bSet)
				res.Import(m_AddPt);
			else
			{
				Point::Native pt;
				pt.Import(m_AddPt);
				res.Add(pt);
			}

			NoLeak<Scalar::Native> k2;
			k2.V.SetSum(k, m_AddScalar);

			Generator::SetMul(res, false, m_pPts, nLevels, k2.V);
		}

		void Obscured::SetMul(Point::Native& res, bool bSet, const Scalar& k) const
		{
			NoLeak<Scalar::Native> k2;
			k2.V.Import(k); // don't care if overflown (still valid operation)
			SetMul(res, bSet, k2.V);
		}

	} // namespace Generator

	/////////////////////
	// Context
	Context::Context()
		:G("G-gen")
		,H("H-gen")
	{
	}

	void Context::Excess(Point::Native& res, const Scalar::Native& k) const
	{
		G.SetMul(res, true, k);
	}

	void Context::Commit(Point::Native& res, const Scalar::Native& k, const Scalar::Native& v) const
	{
		Excess(res, k);
		H.SetMul(res, false, v);
	}

	void Context::Commit(Point::Native& res, const Scalar::Native& k, const Amount& v, Scalar::Native& vOut) const
	{
		vOut.Set(v);
		Commit(res, k, vOut);
	}

	void Context::Commit(Point::Native& res, const Scalar::Native& k, const Amount& v) const
	{
		NoLeak<Scalar::Native> vOut;
		Commit(res, k, v, vOut.V);
	}

	/////////////////////
	// Oracle
	void Oracle::Reset()
	{
		m_hp.Reset();
	}

	void Oracle::GetChallenge(Scalar::Native& out)
	{
		Hash::Value hv; // not secret
		m_hp.Finalize(hv);
		out.ImportFix(hv);
	}

	void Oracle::Add(const void* p, uint32_t n)
	{
		m_hp.Write(p, n);
	}

	void Oracle::Add(const uintBig& v)
	{
		Add(v.m_pData, sizeof(v.m_pData));
	}

	/////////////////////
	// Signature
	void Signature::get_Challenge(Scalar::Native& out, const Hash::Value& msg) const
	{
		Oracle oracle;
		oracle.Add(msg);
		oracle.Add(m_NonceX);
		oracle.GetChallenge(out);
	}

	void Signature::ValFromPt(uintBig& out, const Point::Native& pt)
	{
		Point pt2;
		pt.Export(pt2);
		out = pt2.m_X;
	}

	void Signature::Create(const Hash::Value& msg, const Scalar::Native& sk)
	{
		NoLeak<Scalar::Native> nonce;
		{
			NoLeak<Scalar> s0;
			s0.V.SetRandom();
			nonce.V.ImportFix(s0.V.m_Value);

			Point::Native pt; // not secret
			Context::get().Excess(pt, nonce.V);

			ValFromPt(m_NonceX, pt);
		}

		Scalar::Native sig;
		get_Challenge(sig, msg);

		sig.Mul(sk);
		sig.Add(nonce.V);

		sig.Export(m_Value);
	}

	bool Signature::IsValid(const Hash::Value& msg, const Point::Native& pk) const
	{
		Scalar::Native sig;
		sig.Import(m_Value);

		Point::Native pt;
		Context::get().Excess(pt, sig);

		get_Challenge(sig, msg);
		Scalar e;
		sig.Export(e);

		Point::Native pt2 = pk;
		pt2.Neg();
		pt2.Mul(e);

		pt.Add(pt2);

		uintBig val;
		ValFromPt(val, pt);

		return val == m_NonceX;
	}

} // namespace ECC