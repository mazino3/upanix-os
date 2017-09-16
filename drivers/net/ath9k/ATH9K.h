#pragma once

#include <NetworkDevice.h>

class PCIEntry;

class ATH9K : public NetworkDevice
{
  public:
    ATH9K(PCIEntry& pciEntry);
    ~ATH9K() {}
//void ath9k_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control, struct sk_buff *skb)
  virtual void Tx();
//int ath9k_start(struct ieee80211_hw *hw)
  virtual int Start();
//void ath9k_stop(struct ieee80211_hw *hw)
  virtual void Stop();
//int ath9k_add_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
  virtual int AddInterface();
//int ath9k_change_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif, enum nl80211_iftype new_type, bool p2p)
  virtual int ChangeInterface();
//void ath9k_remove_interface(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
  virtual void RemoveInterface();
//int ath9k_config(struct ieee80211_hw *hw, u32 changed)
  virtual int Configure();
//void ath9k_configure_filter(struct ieee80211_hw *hw, unsigned int changed_flags, unsigned int *total_flags, u64 multicast)
  virtual void ConfigureFilter();
//int ath9k_sta_state(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta, enum ieee80211_sta_state old_state, enum ieee80211_sta_state new_state)
  virtual int STAState();
//void ath9k_sta_notify(struct ieee80211_hw *hw, struct ieee80211_vif *vif, enum sta_notify_cmd cmd, struct ieee80211_sta *sta)
  virtual void STANotify();
//int ath9k_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u16 queue, const struct ieee80211_tx_queue_params *params)
  virtual void ConfigureTx();
//void ath9k_bss_info_changed(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_bss_conf *bss_conf, u32 changed)
  virtual void BSSInfoChanged();
//int ath9k_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd, struct ieee80211_vif *vif, struct ieee80211_sta *sta, struct ieee80211_key_conf *key)
  virtual int SetKey();
//u64 ath9k_get_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
	virtual uint64_t GetTSF();
//void ath9k_set_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u64 tsf)
  virtual void SetTFS();
//void ath9k_reset_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
	virtual void ResetTSF();
//int ath9k_ampdu_action(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_ampdu_params *params)
	virtual int AMPDUAction();
//int ath9k_get_survey(struct ieee80211_hw *hw, int idx, struct survey_info *survey)
	virtual int GetSurvey();
//void ath9k_set_coverage_class(struct ieee80211_hw *hw, s16 coverage_class)
	virtual void SetCoverageClass();
//void ath9k_flush(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u32 queues, bool drop)
	virtual void Flush();
//bool ath9k_tx_frames_pending(struct ieee80211_hw *hw)
	virtual bool TxFramesPending();
//int ath9k_tx_last_beacon(struct ieee80211_hw *hw)
	virtual int TxLastBeacon();
//int ath9k_get_stats(struct ieee80211_hw *hw, struct ieee80211_low_level_stats *stats) static int ath9k_set_antenna(struct ieee80211_hw *hw, u32 tx_ant, u32 rx_ant)
	virtual int GetStats();
//int ath9k_get_antenna(struct ieee80211_hw *hw, u32 *tx_ant, u32 *rx_ant)
	virtual int GetAntenna();
//void ath9k_release_buffered_frames(struct ieee80211_hw *hw, struct ieee80211_sta *sta, u16 tids, int nframes, enum ieee80211_frame_release_type reason, bool more_data)
	virtual void ReleaseBufferedFrames();
//void ath9k_sw_scan_start(struct ieee80211_hw *hw, struct ieee80211_vif *vif, const u8 *mac_addr)
	virtual void SWScanStart();
//void ath9k_sw_scan_complete(struct ieee80211_hw *hw, struct ieee80211_vif *vif) static int ath9k_get_txpower(struct ieee80211_hw *hw, struct ieee80211_vif *vif, int *dbm)
	virtual void SWScanComplete();
//void ath9k_wake_tx_queue(struct ieee80211_hw *hw, struct ieee80211_txq *queue)
	virtual void WakeTxQueue();
};
