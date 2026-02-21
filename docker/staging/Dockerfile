# =========================
# Build stage
# =========================
FROM node:24.11.0 AS build-stage

WORKDIR /app

# 1️⃣ Copier uniquement les manifests du projet docs
COPY docs/package*.json ./docs/

# 2️⃣ Installer les deps
WORKDIR /app/docs
RUN npm ci

# 3️⃣ Copier le reste du projet docs
COPY docs/ ./

# 4️⃣ Build VitePress
RUN npm run build

# =========================
# NGINX stage
# =========================
FROM nginx:stable-alpine AS staging-stage

# ⚠️ ICI on copie .vitepress/dist
COPY --from=build-stage /app/docs/.vitepress/dist /usr/share/nginx/html

# Config nginx
COPY ./docker/nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]