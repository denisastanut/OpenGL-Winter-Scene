#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 pointLightPos1;
uniform vec3 pointLightPos2;
uniform vec3 pointLightPos3;
uniform vec3 pointLightPos4;
uniform vec3 pointLightColor;
uniform vec3 fireLightPos;
uniform vec3 fireLightColor;
uniform vec3 secondFireLightPos;
uniform vec3 secondFireLightColor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

uniform int fogInit;
uniform float is_light;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;


float computeShadow()
{
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5;
    
    if (normalizedCoords.z > 1.0f) return 0.0f;

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;
    float bias = 0.005f;
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

    return shadow;
}


void computeDirLight()
{
    vec3 normalEye = normalize(fNormal); 
    vec3 lightDirN = normalize(lightDir);
    vec3 viewDir = normalize(-fPosEye.xyz);

    ambient = ambientStrength * lightColor;

    float diff = max(dot(normalEye, lightDirN), 0.0f);
    diffuse = diff * lightColor;

    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    specular = specularStrength * spec * lightColor;

    float shadow = computeShadow();
    diffuse *= (1.0f - shadow);
    specular *= (1.0f - shadow);
}


vec3 computePointLight(vec3 lightPosWorld, vec3 color) 
{
    vec4 lightPosEye = view * vec4(lightPosWorld, 1.0f);
    vec3 lightVec = lightPosEye.xyz - fPosEye.xyz; 
    
    float dist = length(lightVec);
    vec3 lightDirN = normalize(lightVec);
    
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));
    
    vec3 amb = 0.05f * color; 
    
    vec3 norm = normalize(fNormal);
    
    float diff = max(dot(norm, lightDirN), 0.0f);
    vec3 dif = diff * color;
    
    vec3 viewDir = normalize(-fPosEye.xyz);
    vec3 reflectDir = reflect(-lightDirN, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    vec3 spe = spec * color; 
    
    return (amb + dif + spe) * att;
}


float computeFog()
{
    float fogDensity = 0.005f;
    float fragmentDistance = length(fPosEye.xyz); 
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return clamp(fogFactor, 0.0f, 1.0f);
}


void main() 
{
    computeDirLight();

    vec3 pointLights = vec3(0.0f);

    pointLights += computePointLight(pointLightPos1, pointLightColor);
    pointLights += computePointLight(pointLightPos2, pointLightColor);
    pointLights += computePointLight(pointLightPos3, pointLightColor);
    pointLights += computePointLight(pointLightPos4, pointLightColor);

    pointLights += computePointLight(secondFireLightPos, secondFireLightColor);
    pointLights += computePointLight(fireLightPos, fireLightColor);

    vec3 texColor = texture(diffuseTexture, fTexCoords).rgb;
    if(texture(diffuseTexture, fTexCoords).a < 0.1) discard;

    vec3 color = min((ambient + diffuse) * texColor + specular + pointLights * texColor, 1.0f);

    if (fogInit == 1) 
    {
        float fogFactor = computeFog();
        vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f); 
        fColor = mix(fogColor, vec4(color, 1.0f), fogFactor);
    }
    else 
    {
        fColor = vec4(color, 1.0f);
    }
}