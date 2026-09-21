[简体中文](SECURITY.zh_CN.md) · **English**

# Security policy

StreetPass exchanges public profile cards. It does not authenticate identities or provide confidential messaging. Do not put secrets in a public card.

## Report privately

Report vulnerabilities in this application to the maintainer of **VictorTran1023/ai-passport-streetpass**, not to the upstream project by default. Use [private vulnerability reporting](https://github.com/VictorTran1023/ai-passport-streetpass/security/advisories/new) when available. If unavailable, open an issue requesting a private contact without including exploit details, credentials, personal data or reproduction materials.

Include the firmware commit, board revision, affected component, impact and sanitized reproduction steps through the private channel. For a confirmed upstream BSP issue, coordinate with the maintainer before involving FoloToy. This prototype has no guaranteed security response time or support period.

## Scope and disclosure

Hotspot authorization, editor input handling, BLE framing and persistent storage are relevant security surfaces. BLE checksums detect corruption; they do not prevent malicious impersonation. Removing published fields cannot erase previously received copies.

Please coordinate disclosure until a fix or mitigation can be assessed. Never attach actual device QR credentials or private card data to public issues or commits.
