
{% for device in devices -%}
# include <{{ device.class_name|lower + '.hpp' }}>
{% endfor %}