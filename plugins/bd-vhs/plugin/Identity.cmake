# ============================================================================
#  DO NOT CHANGE ANYTHING IN THIS FILE.
#
#  Every value here feeds a plugin identifier that hosts store in saved
#  sessions. In particular the VST3 FUID is derived from the manufacturer code,
#  the plugin code AND the product name string -- so even an innocent-looking
#  rename silently invalidates every project that has ever loaded this plugin,
#  and the user gets "missing plugin" with no explanation.
#
#  If you genuinely need to change one of these, treat it as shipping a new
#  plugin, not as editing this one.
# ============================================================================

set(BDVHS_PRODUCT_NAME     "BD-VHS")
set(BDVHS_COMPANY_NAME     "xbraindance")
set(BDVHS_COMPANY_WEBSITE  "https://github.com/xbraindance")
set(BDVHS_BUNDLE_ID        "com.xbraindance.bdvhs")

# Four characters each, at least one uppercase -- an Audio Unit requirement.
set(BDVHS_MANUFACTURER_CODE Xbdn)
set(BDVHS_PLUGIN_CODE       Bvhs)

set(BDVHS_CLAP_ID          "com.xbraindance.bd-vhs")
