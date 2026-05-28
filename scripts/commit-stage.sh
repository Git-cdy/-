#!/usr/bin/env bash
# scripts/commit-stage.sh
# 用法: bash scripts/commit-stage.sh <step号> "<描述>"
# 例:   bash scripts/commit-stage.sh 0 "时钟+调度器+串口验证通过"

set -e

STEP="$1"
MSG="$2"

if [ -z "$STEP" ] || [ -z "$MSG" ]; then
  echo "用法: bash scripts/commit-stage.sh <step号 0-11> \"<描述>\""
  echo "例:  bash scripts/commit-stage.sh 0 \"时钟+调度器+串口验证通过\""
  exit 1
fi

TAG="step${STEP}-passed"

# 检查 tag 是否已存在
if git rev-parse "$TAG" >/dev/null 2>&1; then
  echo "⚠️  tag '$TAG' 已存在。如果你确定要重做这一步，先删除旧 tag:"
  echo "    git tag -d $TAG && git push origin :refs/tags/$TAG"
  exit 1
fi

echo "==> [1/4] git add ."
git add .

echo "==> [2/4] git commit"
git commit -m "step${STEP}: ${MSG}" || { echo "（无改动可提交，跳过 commit）"; }

echo "==> [3/4] 打 tag: $TAG"
git tag -a "$TAG" -m "第 ${STEP} 步通过: ${MSG}"

echo "==> [4/4] push 到 GitHub (含 tag)"
if git remote get-url origin >/dev/null 2>&1; then
  git push origin main
  git push origin "$TAG"
  echo "✅ 已 push 到远程。回滚命令: git checkout $TAG"
else
  echo "⚠️  尚未配置 remote origin，仅完成本地 commit + tag"
  echo "    配置远程: git remote add origin <你的 GitHub 仓库 URL>"
fi

echo ""
echo "完成。当前所有 tag:"
git tag -l "step*-passed"
