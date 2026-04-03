#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5,1.0,0.3));
    float NdotL = max(dot(normalize(fragNormal), lightDir), 0.0);

    vec3 ambient = fragColor.rgb * 0.15;
    vec3 diffuse = fragColor.rgb * NdotL;

    outColor = vec4(ambient + diffuse, fragColor.a);
}