if ( grep -qs '^root::' /etc/shadow && \
     [ -z "$FAILSAFE" ] )
then
	local default_pw='root:$1$eCzRcxZH$Uam6IfyVZg3iRBZjBGDNl\/:17997:'
	sed -i "s/^root::0:/${default_pw}/g" /etc/shadow
cat << EOF
=== WARNING! =====================================
The default root password created on this device!
--------------------------------------------------
EOF

fi
