/// Generated by Devgen 
/// DO NOT EDIT
///
/// (c) Koheron 

{% for device in devices -%}
{% for include in device.includes -%}
#include "{{ include }}"
{% endfor -%}
{% endfor %}