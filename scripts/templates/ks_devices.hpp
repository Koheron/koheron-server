
{% for device in devices -%}
# include <{{ device.ks_name + '.hpp' }}>
{% endfor %}