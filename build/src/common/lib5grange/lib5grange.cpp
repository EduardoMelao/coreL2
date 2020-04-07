#include "lib5grange.h"

namespace lib5grange{
	size_t
	get_re_capacity(
		const size_t & numID,                 // Numerology ID
		const allocation_cfg_t & allocation,  // Allocation config struct
		const mimo_cfg_t & mimo)              // MIMO config struct
	{
		size_t numRB = allocation.number_of_rb;
		if (numRB==0){numRB=1;}
		const size_t subcarriers = numerology[numID].subcarriers_per_rb;
		const size_t subsymbols  = numerology[numID].m;
		const size_t symbols     = numerology[numID].symbols_per_subframe;
		const size_t df          = numerology[numID].pilot_df;
		const size_t dt          = numerology[numID].pilot_dt;
		size_t numRE = subcarriers * subsymbols * symbols;
		numRE = numRE - numRE/(df*dt);                                          // Account for REs used for pilots
		numRE = (numRE * numRB) - DCI_SIZE - DCI_SIZE*((numRB-1)/NUM_TB_PER_DCI);   // Account for REs used for DCI
		if (mimo.scheme==MULTIPLEXING){
			numRE = numRE * mimo.num_tx_antenas;
		}
		return numRE;
	}

	size_t
	get_bit_capacity(
		const size_t & numID,
		const allocation_cfg_t & allocation,
		const mimo_cfg_t & mimo,
		const qammod_t & mod)
	{
		size_t numRE = get_re_capacity(numID, allocation, mimo);
		size_t numBits = (numRE*mod);
		return numBits;
	}

	size_t
	get_bit_capacity(
		const size_t & numRE, // Number of resource elements
		const qammod_t & mod) // Modulation
	{
		return (numRE*mod);
	}

	size_t
	get_num_required_rb(
		const size_t & numID,    // Numerology ID
		const mimo_cfg_t & mimo, // MIMO Onfiguration
		const qammod_t & mod,    // Modulation
		float target_coderate,   // Target coderate
		size_t info_bits)        // number of information bits
	{
		float required_bit_capacity = round((float)info_bits/target_coderate);
		allocation_cfg_t aloc;
		aloc.target_ue_id =0;
		aloc.first_rb=0;
		aloc.number_of_rb=1;
		size_t gross_rb_bit_capacity = get_bit_capacity(numID,aloc,mimo,mod) + (DCI_SIZE*mod);
		size_t gross_qam_capacity = gross_rb_bit_capacity/mod;
		size_t num_qam_required = (size_t) round(required_bit_capacity / mod); //round odr ceil
		num_qam_required += DCI_SIZE;   // Account for the first DCI
		num_qam_required += (((num_qam_required-DCI_SIZE)/gross_qam_capacity))/NUM_TB_PER_DCI * DCI_SIZE; // Account for extra DCIs
		size_t numRB = (size_t) ceil(float(num_qam_required)/float(gross_qam_capacity));
		return numRB;
	} /* get_mum_required_rb */

	MacPDU::MacPDU(){
		numID_ = 0;
		mac_data_   = {};
		snr_avg_ = 10;
		rankIndicator_ = 10;
	}


	MacPDU::MacPDU(
		unsigned numID,                 // Numerology ID
		macphyctl_t phyctl,             // phyctl MAC/PHY control struct
		allocation_cfg_t allocation,    // allocation config
		mimo_cfg_t mimo_cfg,            // MIMO config
		mcs_cfg_t mcs_cfg)              // modulation and coding config
	{
		numID_ = numID;
		macphy_ctl_ = phyctl;
		allocation_ = allocation;
		mimo_ = mimo_cfg;
		mcs_ = mcs_cfg;
		snr_avg_ = 10;
		rankIndicator_ = 10;

	} /*MacPDU()*/



	void
	MacPDU::serialize(vector<uint8_t> & bytes)
	{
		push_bytes(bytes, numID_);
		macphy_ctl_.serialize(bytes);
		allocation_.serialize(bytes);
		mimo_.serialize(bytes);
		mcs_.serialize(bytes);
		push_bytes(bytes, snr_avg_);
		push_bytes(bytes, rankIndicator_);
		serialize_vector(bytes, mac_data_);
	}

	MacPDU::MacPDU(vector<uint8_t> & bytes)
	{
		deserialize_vector(mac_data_, bytes);
		pop_bytes(snr_avg_, bytes);
		pop_bytes(rankIndicator_, bytes);
		mcs_.deserialize(bytes);
		mimo_.deserialize(bytes);
		allocation_.deserialize(bytes);
		macphy_ctl_.deserialize(bytes);
		pop_bytes(numID_, bytes);
		snr_avg_ = 10;
		rankIndicator_ = 10;
	}
}//namespace lib5grange
